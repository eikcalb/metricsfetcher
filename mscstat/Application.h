#pragma once
#include <string>
#include <csignal>

#include "LogManager.h"
#include "ConfigManager.h"
#include "ThreadManager.h"
#include "DataManager.h"
#include "MetricsManager.h"

class Application
{
public:
    void Run();

    static Application& CreateInstance() {
        if (!theApp) {
            theApp = std::make_shared<Application>();
        }

        return *theApp;
    }

    Application(): configManager(nullptr), dataManager(nullptr), logManager(nullptr), metricsManager(nullptr), threadManager(nullptr) {
    }

    ~Application() {
        logManager->LogInfo("Application Ended!");
    }

    void Initialize();

private:
    static void SignalHandler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            g_signal_flag = true;
        }
    }

    // Creates the root application table that is responsible for storing application
    // specific information. This includes configuration details, users, keys and any
    // other information.
    // 
    // This table mimicks a key-value store.
    void CreateApplicationTable() {
        dataManager->CreateTable(tableName, "id INTEGER PRIMARY KEY, key TEXT UNIQUE NOT NULL, value TEXT");
    }

    std::string FetchConfigData() {
        auto resultSet = dataManager->Select(tableName, "key=\"config\"");
        if (resultSet.empty()) {
            logManager->LogInfo("Could not find configuration in database.");
            logManager->LogInfo("Default config will be used.");
            return "";
        }

        return resultSet.front().GetString(2);
    }


private:
    static volatile std::sig_atomic_t g_signal_flag;
public:
    std::string name = "MSC Fetcher";
    std::string tableName = "MSC_Fetcher";

    ConfigManager* configManager;
    DataManager* dataManager;
    LogManager* logManager;
    MetricsManager* metricsManager;
    ThreadManager* threadManager;

    static std::shared_ptr<Application> theApp;
};
