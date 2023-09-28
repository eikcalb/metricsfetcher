#pragma once
#include <atomic>
#include <chrono>
#include <duktape.h>
#include <stdexcept>
#include <sstream>
#include <vector>
#include "DataManager.h"
#include "MyScript.h"

/**
* ScriptManager is responsible for reading scripts and executing them.
*/
class ScriptManager
{
public:
    static ScriptManager& GetInstance(int intervalMS) {
        static ScriptManager instance(intervalMS);
        return instance;
    }

    void Initialize() {
        // We should retrieve the list of scripts from the DB and create the functions
        auto scriptRows = DataManager::GetInstance().Select("ScriptManager");

        for (auto& row : scriptRows) {
            auto scriptName= row.GetString(1);
            auto scriptText = row.GetString(2);
            auto metricName = row.GetString(3);

            try {
                std::shared_ptr<Script> sc = std::make_shared<Script>(scriptName);
                sc->ExecuteJavaScript(scriptText);
                sc->SetupCounter(metricName);

                duk_push_c_function(sc->GetContext(), [](duk_context* ctx) -> int { 
                    Script* myInstance = static_cast<Script*>(duk_require_pointer(ctx, 0)); // Get the C++ instance
                    int arg1 = duk_require_int(ctx, 1); // Get the argument from JavaScript

                    // Call the C++ class method on the instance
                    myInstance->Persist(arg1);
                    return 0;
                    }, /*num_args=*/1);
                duk_push_pointer(sc->GetContext(), &sc); // Pass a pointer to the C++ instance
                duk_put_prop_string(sc->GetContext(), -2, "\xFF" "myInstance"); // Set a property to store the C++ instance
                duk_put_global_string(sc->GetContext(), "persist");

                duk_push_c_function(sc->GetContext(), [](duk_context* ctx) -> int {
                    Script* myInstance = static_cast<Script*>(duk_require_pointer(ctx, 0)); // Get the C++ instance

                    // Call the C++ class method on the instance
                    auto counterValue = myInstance->GetCounterValue();
                    duk_push_number(ctx, counterValue);
                    return 1;
                    }, /*num_args=*/0);
                duk_push_pointer(sc->GetContext(), &sc); // Pass a pointer to the C++ instance
                duk_put_prop_string(sc->GetContext(), -2, "\xFF" "myInstance2"); // Set a property to store the C++ instance
                duk_put_global_string(sc->GetContext(), "getCounterValue");

                scripts.emplace_back(sc);
            }
            catch (std::exception e) {
                // Ignore error and move to next script
                LogManager::GetInstance().LogError("Failed to execute script: {0}", row.GetString(1));
            }
        }
    }

    // Save JavaScript function bytecode to the database
    void SaveScript(const std::string name, const std::string& script, const std::string metricName) {
        const auto p1 = std::chrono::system_clock::now();

        // Create a stringstream object
        std::ostringstream stream{};
        stream << "NULL, "
            << "\"" << name << "\", "
            << "\"" << script << "\", "
            << "\"" << metricName << "\", "
            << std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
        std::string sqlString = stream.str();

        if (DataManager::GetInstance().Insert("ScriptManager", sqlString)) {
            LogManager::GetInstance().LogInfo("Saved script to the database: {0}", name);
        }
    }

    void Process();

    void Stop() {
        should_stop.store(true);
    }

private:
    ScriptManager(int intervalMS): intervalMS_(intervalMS) {
        should_stop.store(false);
        DataManager::GetInstance().CreateTable("ScriptManager", " \
            id INTEGER PRIMARY KEY, \
            name TEXT NOT NULL, \
            scriptText TEXT NOT NULL, \
            metricName TEXT DEFAULT NULL, \
            timestamp INTEGER NOT NULL"
        );
    }

private:
    std::vector<std::shared_ptr<Script>> scripts;
    std::atomic<bool> should_stop;
    int intervalMS_;
};
