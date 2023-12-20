#pragma once
#include <atomic>
#include <chrono>
#include <duktape.h>
#include <stdexcept>
#include <sstream>
#include <pdh.h>
#include <pdhmsg.h>

#include "DataManager.h"
#include "LogManager.h"

class Script {
private:
    std::string getLastErrorAsString()
    {
        //Get the error message ID, if any.
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0) {
            return std::string(); //No error message has been recorded
        }

        LPSTR messageBuffer = nullptr;

        //Ask Win32 to give us the string version of that message ID.
        //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        //Copy the error message into a std::string.
        std::string message(messageBuffer, size);

        //Free the Win32's string's buffer.
        LocalFree(messageBuffer);

        return message;
    }
public:
    Script(std::string scriptName, std::string text, std::string scriptMetricName) : scriptText(text) {
        name = scriptName;
        metricName = scriptMetricName;

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

        SetupCounter();
    }

    ~Script() {
        // Destroy the Duktape context
        //if (ctx) {
        //    duk_destroy_heap(ctx);
        //}

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
            duk_get_prop_string(ctx, -1, "stack");
            auto err = duk_safe_to_string(ctx, -1);
            if (err) {
                throw std::runtime_error(err);
            }
            else {
                throw std::runtime_error("JavaScript execution error");
            }
        }

        // Ignore the return value
        duk_pop(ctx);
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
            duk_get_prop_string(ctx, -1, "stack");
            auto err = duk_safe_to_string(ctx, -1);
            if (err) {
                throw std::runtime_error(err);
            }
            else {
                throw std::runtime_error("JavaScript function call error");
            }
        }

        // Ignore the return value
        duk_pop(ctx);
    }

    void ClearDuktapeStack() {
        // Get the current stack size
        duk_idx_t stackSize = duk_get_top(ctx);

        // Pop all items from the stack
        duk_pop_n(ctx, stackSize);
        duk_destroy_heap(ctx);
    }

    void CreateContext() {
        ctx = duk_create_heap_default();


        if (!ctx) {
            throw std::runtime_error("Failed to create Duktape context.");
        }
    }

    void SetupCounter() {
        const auto counterName = Utils::StringToWstring(metricName);
        PdhAddEnglishCounter(queryHandle, counterName.c_str(), 0, &counter);

        PdhCollectQueryData(queryHandle);
    }

    void Persist(double value) {
        const auto p1 = std::chrono::system_clock::now();

        // Create a stringstream object
        std::ostringstream stream{};
        stream << "NULL, "
            << metricCounter << ", "
            << "\"" << name << "\"" << ", "
            << value << ", "
            << std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
        std::string sqlString = stream.str();
        std::cout << metricCounter;
        DataManager::GetInstance().Insert("ScriptData", sqlString);
    };

    double GetCounterValue() {
        PDH_FMT_COUNTERVALUE value;
        PDH_STATUS status;

        status = PdhCollectQueryData(queryHandle);
        if (status != ERROR_SUCCESS) {
            LogManager::GetInstance().LogWarning("Failed to read counter value: {0}", getLastErrorAsString());
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

    void LogDebugContext() {
        const duk_idx_t top = duk_get_top(ctx);

        for (duk_idx_t i = 0; i < top; ++i) {
            duk_dup(ctx, i);
            duk_json_encode(ctx, -1);
            auto* str = duk_safe_to_string(ctx, -1);
            if (str) {
                LogManager::GetInstance().LogDebug("Duktape Stack: {0}, {1}", str, i);
            }
            duk_pop(ctx);
        }
    }

private:
    PDH_HCOUNTER counter = nullptr;
    PDH_HQUERY queryHandle = nullptr;

    duk_context* ctx = nullptr;

    std::string name;
    std::string scriptText;
    std::string metricName;

public:
    std::mutex ctxMutex;
    UINT metricCounter = 0;
};
