#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <duktape.h>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

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
            auto scriptName= row.GetString("name");
            auto scriptText = row.GetString("scriptText");
            auto metricName = row.GetString("metricName");

            try {
                std::shared_ptr<Script> sc = std::make_shared<Script>(scriptName, scriptText, metricName);

                SetupJavascriptContext(sc);

                scripts.emplace_back(sc);
            }
            catch (std::exception e) {
                // Ignore error and move to next script
                LogManager::GetInstance().LogError("Failed to execute script: {0}", row.GetString("name"));
            }
        }
    }

    std::string GetAllScriptsAsJSON() {
        // Create a RapidJSON Document
        rapidjson::Document doc;
        rapidjson::Value jsonArray(rapidjson::kArrayType);
        doc.SetObject();

        for (const auto& script : scripts) {
            const auto [name, scriptText, metricName] = script->GetInfo();
            rapidjson::Value obj(rapidjson::kObjectType);

            rapidjson::Value name_;
            name_.SetString(name.c_str(), doc.GetAllocator());
            obj.AddMember("name", name_, doc.GetAllocator());

            rapidjson::Value script_;
            script_.SetString(scriptText.c_str(), doc.GetAllocator());
            obj.AddMember("scriptText", script_, doc.GetAllocator());
            
            rapidjson::Value metric_;
            metric_.SetString(metricName.c_str(), doc.GetAllocator());
            obj.AddMember("metricName", metric_, doc.GetAllocator());

            jsonArray.PushBack(obj, doc.GetAllocator());
        }

        doc.AddMember("scripts", jsonArray, doc.GetAllocator());

        // Serialize the Document to a JSON string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        buffer.Flush();
        doc.Accept(writer);

        return buffer.GetString();
    }

    // Save JavaScript function to the database
    bool SaveScript(const std::string& jsonString) {
        const auto p1 = std::chrono::system_clock::now();

        std::string name;
        std::string scriptText;
        std::string metricName;
        rapidjson::Document document;

        if (jsonString.empty()) {
            return false;
        }

        document.Parse(jsonString.c_str(), jsonString.length());

        if (!document.IsObject() || document.HasParseError()) {
            std::cerr << "Invalid JSON format." << document.GetParseError() << std::endl;
            throw std::runtime_error("JSON is in wrong format." + document.HasParseError() ? " " + document.GetParseError() : "");
        }

        if (document.HasMember("name") && document["name"].IsString()) {
            name = document["name"].GetString();
        }
        if (document.HasMember("scriptText") && document["scriptText"].IsString()) {
            scriptText = document["scriptText"].GetString();
        }
        if (document.HasMember("metricName") && document["metricName"].IsString()) {
            metricName = document["metricName"].GetString();
        }

        // Create a stringstream object
        std::ostringstream stream{};
        stream << "NULL, "
            << "\"" << name << "\", "
            << "\"" << scriptText << "\", "
            << "\"" << metricName << "\", "
            << std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
        std::string sqlString = stream.str();

        if (DataManager::GetInstance().Insert("ScriptManager", sqlString)) {
            LogManager::GetInstance().LogInfo("Saved script to the database: {0}", name);
            // Add new script to in-memory list of scripts
            std::shared_ptr<Script> sc = std::make_shared<Script>(name, scriptText, metricName);
            scripts.emplace_back(sc);

            return true;
        }

        return false;
    }

    bool DeleteScript(const std::string& name) {
        LogManager::GetInstance().LogInfo("Deleting script from the database.");

        if (name.empty()) {
            throw std::runtime_error("Script name must be provided for deletion.");
        }

        if (DataManager::GetInstance().Delete("ScriptManager", "name = \"" + name + "\"")) {
            LogManager::GetInstance().LogInfo("Deleted script from the database: {0}", name);
            auto nameComparator = [&name](const std::shared_ptr<Script>& script) {
                return script->GetInfo()[0] == name;
            };
            auto newEnd = std::remove_if(scripts.begin(), scripts.end(), nameComparator);
            scripts.erase(newEnd, scripts.end());
            return true;
        }

        return false;
    }

    void Process(UINT64 counter);

    void Stop() {
        should_stop.store(true);
    }

private:
    ScriptManager(int intervalMS): intervalMS_(intervalMS) {
        should_stop.store(false);
        DataManager::GetInstance().CreateTable("ScriptManager", " \
            id INTEGER PRIMARY KEY, \
            name TEXT UNIQUE NOT NULL, \
            scriptText TEXT NOT NULL, \
            metricName TEXT DEFAULT NULL, \
            timestamp INTEGER NOT NULL"
        );
    }

    void SetupJavascriptContext(std::shared_ptr<Script>& sc) {
        sc->SetupCounter();
        sc->ExecuteJavaScript();

        duk_push_c_function(sc->GetContext(), [](duk_context* ctx) -> int {
            duk_push_this(ctx);
            Script* myInstance = static_cast<Script*>(duk_require_pointer(ctx, 0)); // Get the C++ instance
            double arg1 = duk_require_number(ctx, 1); // Get the argument from JavaScript

            // Call the C++ class method on the instance
            myInstance->Persist(arg1);
            return 0;
        }, /*num_args=*/1);
        duk_push_pointer(sc->GetContext(), &sc); // Pass a pointer to the C++ instance
        // duk_put_prop_string(sc->GetContext(), -2, "\xFF" "myInstance"); // Set a property to store the C++ instance
        duk_put_global_string(sc->GetContext(), "persist");

        duk_push_c_function(sc->GetContext(), [](duk_context* ctx) -> int {
            Script* myInstance = static_cast<Script*>(duk_require_pointer(ctx, 0)); // Get the C++ instance

            // Call the C++ class method on the instance
            auto counterValue = myInstance->GetCounterValue();
            duk_push_number(ctx, counterValue);
            return 1;
            }, /*num_args=*/0);
        duk_push_pointer(sc->GetContext(), &sc); // Pass a pointer to the C++ instance
        // duk_put_prop_string(sc->GetContext(), -2, "\xFF" "myInstance2"); // Set a property to store the C++ instance
        duk_put_global_string(sc->GetContext(), "getCounterValue");
    }

private:
    std::vector<std::shared_ptr<Script>> scripts;
    std::atomic<bool> should_stop;
    int intervalMS_;
};
