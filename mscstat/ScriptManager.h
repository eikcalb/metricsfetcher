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
#include "Script.h"

#ifndef SCRIPT_INSTANCE_NAME
#define SCRIPT_INSTANCE_NAME DUK_HIDDEN_SYMBOL("instance")
#endif // !SCRIPT_INSTANCE_NAME


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

        std::lock_guard<std::mutex> lock(scriptMutex);
        for (auto& row : scriptRows) {
            auto scriptName = row.GetString("name");
            auto scriptText = row.GetString("scriptText");
            auto metricName = row.GetString("metricName");

            try {
                std::shared_ptr<Script> sc = std::make_shared<Script>(scriptName, scriptText, metricName);

                //SetupJavascriptContext(sc);
                scripts.push_back(std::move(sc));
            }
            catch (std::exception e) {
                // Ignore error and move to next script
                LogManager::GetInstance().LogError("Failed to execute script: {0}", row.GetString("name"));
            }
        }
    }

    std::string GetAllScriptsAsJSON() const {
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

    void updateDataJSONArray(rapidjson::Document& doc, rapidjson::Value& response, const UINT8& count) const {
        // Fetch the most recent data, up to `count`
        for (auto& script : scripts) {
            const auto& rows = DataManager::GetInstance().Select("ScriptData", "key=\"" + script->GetInfo()[0] + "\" ORDER BY id DESC LIMIT " + std::to_string(count));

            rapidjson::Value obj(rapidjson::kObjectType);

            rapidjson::Value name;
            name.SetString(script->GetInfo()[0].c_str(), doc.GetAllocator());
            obj.AddMember("name", name, doc.GetAllocator());

            rapidjson::Value isCustom;
            isCustom.SetBool(true);
            obj.AddMember("isCustom", isCustom, doc.GetAllocator());

            rapidjson::Value dataArr(rapidjson::kArrayType);

            for (auto& row : rows) {
                rapidjson::Value data(rapidjson::kObjectType);

                auto id = row.GetInt("id");
                auto counter = row.GetInt("counter");
                auto timestamp = row.GetInt("timestamp");
                auto value = row.GetDouble("value");

                data.AddMember("id", Utils::ConvertIntToJSONValue(id, doc.GetAllocator()), doc.GetAllocator());
                data.AddMember("counter", Utils::ConvertIntToJSONValue(counter, doc.GetAllocator()), doc.GetAllocator());
                data.AddMember("timestamp", Utils::ConvertIntToJSONValue(timestamp, doc.GetAllocator()), doc.GetAllocator());
                data.AddMember("value", Utils::ConvertDoubleToJSONValue(value, doc.GetAllocator()), doc.GetAllocator());

                dataArr.PushBack(data, doc.GetAllocator());
            }

            obj.AddMember("data", dataArr, doc.GetAllocator());

            response.PushBack(obj, doc.GetAllocator());
        }
    };

    rapidjson::Value GetAggregateDataJSON(rapidjson::Document& doc, const std::string& name) const {
        rapidjson::Value obj(rapidjson::kObjectType);

        const auto& rows = DataManager::GetInstance().SelectAggregate("ScriptData", "value", "key=\"" + name + "\"");

        const auto& row = rows[0];
        auto max = row.GetDouble("max");
        auto min = row.GetDouble("min");
        auto avg = row.GetDouble("avg");
        auto total = row.GetDouble("total");
        auto count = row.GetInt("count");

        obj.AddMember("max", Utils::ConvertDoubleToJSONValue(max, doc.GetAllocator()), doc.GetAllocator());
        obj.AddMember("min", Utils::ConvertDoubleToJSONValue(min, doc.GetAllocator()), doc.GetAllocator());
        obj.AddMember("avg", Utils::ConvertDoubleToJSONValue(avg, doc.GetAllocator()), doc.GetAllocator());
        obj.AddMember("total", Utils::ConvertDoubleToJSONValue(total, doc.GetAllocator()), doc.GetAllocator());
        obj.AddMember("count", Utils::ConvertDoubleToJSONValue(count, doc.GetAllocator()), doc.GetAllocator());

        return obj;
    };

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

        if (auto it = std::find_if(scripts.begin(), scripts.end(), [name](const std::shared_ptr<Script>& sc) {return sc->GetInfo()[0] == name; }); it != scripts.end()) {
            // Script with same name already exists
            throw std::runtime_error("Script names must be unique. Provided name already in use.");
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

    bool UpdateScript(const std::string& jsonString) {
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

        auto it = std::find_if(scripts.begin(), scripts.end(), [name](const std::shared_ptr<Script>& sc) {return sc->GetInfo()[0] == name; });
        if (it == scripts.end()) {
            // Script with same name already exists
            throw std::runtime_error("Script not found. The provided script name could not be found");
        }
        if (DataManager::GetInstance().Update("ScriptManager", "\"scriptText\" = \"" + scriptText + "\", \"metricName\" = \"" + metricName + "\"", "\"name\" = \"" + name + "\"")) {
            LogManager::GetInstance().LogInfo("Saved script to the database: {0}", name);
            // Add new script to in-memory list of scripts
            std::shared_ptr<Script> sc = std::make_shared<Script>(name, scriptText, metricName);

            if (it != scripts.end()) {
                // Element with the specified name found, replace it
                *it = sc;
            }
            else {
                LogManager::GetInstance().LogWarning("Failed to update scripts list. A server restart will be required.");
            }

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

    void Process(std::atomic<UINT64>& counter);

    void Stop() {
        should_stop.store(true);
    }

private:
    ScriptManager(int intervalMS) : intervalMS_(intervalMS) {
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
        sc->CreateContext();
        auto* ctx = sc->GetContext();

        duk_push_global_object(ctx);
        duk_push_pointer(ctx, sc.get()); // Pass a pointer to the C++ instance
        duk_put_global_string(ctx, SCRIPT_INSTANCE_NAME); // Set a property to store the C++ instance

        duk_push_c_function(ctx, [](duk_context* ctx_) -> int {
            duk_push_global_object(ctx_);
            duk_get_global_string(ctx_, SCRIPT_INSTANCE_NAME);
            Script* myInstance = static_cast<Script*>(duk_require_pointer(ctx_, -1)); // Get the C++ instance

            double arg1 = duk_require_number(ctx_, 0); // Get the argument from JavaScript

            // Call the C++ class method on the instance
            myInstance->Persist(arg1);

            return 0;
            }, /*num_args=*/1);
        duk_put_global_string(ctx, "persist");

        duk_push_c_function(ctx, [](duk_context* ctx_) -> int {
            duk_push_global_object(ctx_);
            duk_get_global_string(ctx_, SCRIPT_INSTANCE_NAME);
            Script* myInstance = static_cast<Script*>(duk_require_pointer(ctx_, -1)); // Get the C++ instance

            // Call the C++ class method on the instance
            auto counterValue = myInstance->GetCounterValue();

            duk_push_number(ctx_, counterValue);
            return 1;
            }, /*num_args=*/0);
        duk_put_global_string(ctx, "getCounterValue");

        sc->ExecuteJavaScript();
    }

private:
    std::mutex scriptMutex;

    std::vector<std::shared_ptr<Script>> scripts;
    std::atomic<bool> should_stop;
    int intervalMS_;
};
