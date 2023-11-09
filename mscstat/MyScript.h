#pragma once
#include <atomic>
#include <chrono>
#include <duktape.h>
#include <stdexcept>
#include <sstream>
#include <pdh.h>
#include <pdhmsg.h>
#include "DataManager.h"

class Script {
public:
    Script(std::string scriptName, std::string text, std::string scriptMetricName): scriptText(text) {
        name = scriptName;
        metricName = scriptMetricName;

        ctx = duk_create_heap_default();
        if (!ctx) {
            throw std::runtime_error("Failed to create Duktape context.");
        }

        DataManager::GetInstance().CreateTable("ScriptData", " \
            id INTEGER PRIMARY KEY, \
            counter INTEGER NOT NULL, \
            key TEXT NOT NULL, \
            value REAL NOT NULL, \
            timestamp INTEGER NOT NULL"
        );

        PDH_STATUS status = PdhOpenQuery(nullptr, 0, &queryHandle);
        if (status != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to open PDH query.");
        }

    }

    ~Script() {
        // Destroy the Duktape context
        if (ctx) {
            duk_destroy_heap(ctx);
        }

        PdhCloseQuery(queryHandle);
    }

    duk_context* GetContext() {
        return ctx;
    }

    // Function to execute JavaScript code from a string
    void ExecuteJavaScript() {
        if (!ctx) {
            throw std::runtime_error("Duktape context is not initialized.");
        }

        // Execute the JavaScript code
        if (duk_peval_string(ctx, scriptText.c_str()) != 0) {
            auto err = duk_safe_to_string(ctx, -1);
            if (err) {
                throw std::runtime_error(err);
            }
            else {
                throw std::runtime_error("JavaScript execution error");
            }
        }
    }

    // Function to call a JavaScript function by name
    void CallJavaScriptFunction(const std::string& functionName) {
        if (!ctx) {
            throw std::runtime_error("Duktape context is not initialized.");
        }

        // Get the JavaScript function by name
        duk_get_global_string(ctx, functionName.c_str());

        // Call the function with no arguments
        if (duk_pcall(ctx, 0) != 0) {
            auto err = duk_safe_to_string(ctx, -1);
            if (err) {
                throw std::runtime_error(err);
            }
            else {
                throw std::runtime_error("JavaScript function call error");
            }
        }
    }

    void SetupCounter() {
        PdhAddEnglishCounter(queryHandle, LPCWSTR(metricName.c_str()), 0, &counter);
    }

    void Persist(double value) {
        const auto p1 = std::chrono::system_clock::now();

        // Create a stringstream object
        std::ostringstream stream{};
        stream << "NULL, "
            << counter << ", "
            << "\"" << name << "\"" << ", "
            << "\"" << name << "\"" << ", "
            << value << ", "
            << std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
        std::string sqlString = stream.str();

        DataManager::GetInstance().Insert("ScriptData", sqlString);
    };

    double GetCounterValue() {
        PDH_FMT_COUNTERVALUE value;
        PDH_STATUS status;

        status = PdhCollectQueryData(queryHandle);
        if (status != ERROR_SUCCESS) {
            return 0.0;
        }

        PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value);
        if (value.CStatus == PDH_CSTATUS_VALID_DATA) {
            return value.doubleValue;
        }
        else {
            return 0.0;
        }
    }

    std::array<std::string, 3> GetInfo() const {
        return { name, scriptText, metricName };
    }

private:
    PDH_HCOUNTER counter = nullptr;
    PDH_HQUERY queryHandle = nullptr;

    duk_context* ctx = nullptr;

    std::string name;
    std::string scriptText;
    std::string metricName;

public:
    UINT metricCounter = 0;
};
