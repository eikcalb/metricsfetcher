#pragma once
#include <string>
#include <csignal>
#include <stdio.h>

#include "LogManager.h"
#include "ConfigManager.h"
#include "ThreadManager.h"
#include "DataManager.h"
#include "MetricsManager.h"
#include "ScriptManager.h"
#include "Server.h"
#include "IntelligenceManager.h"

class Application
{
public:
    void Run();
    void Initialize();
    void Stop();

    static Application& CreateInstance() {
        if (!theApp) {
            theApp = std::make_shared<Application>();
        }

        return *theApp;
    }

    Application(): configManager(nullptr), dataManager(nullptr), logManager(nullptr), metricsManager(nullptr), threadManager(nullptr) {
    }

    ~Application() {
        std::cout << "Application Ended!";
    }

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
        auto ss = resultSet.front();
        return ss.GetString("value");
    }

public:
    bool SaveConfigData(const std::string & jsonString) {
        auto newConfig = configManager->ParseJSONString(jsonString);
        logManager->LogInfo("Updating application configuration.");
        logManager->LogInfo("Application configuration has been set to: {0}", jsonString);

        return dataManager->Upsert(tableName, "\"key\", \"value\"", "\"config\", \'" + jsonString + "\'");
    }

private:
    static volatile std::sig_atomic_t g_signal_flag;
public:
    std::string version = "1.0";
    std::string name = "Metrics Fetcher";
    std::string tableName = "Metrics_Fetcher";

    ConfigManager* configManager;
    DataManager* dataManager;
    LogManager* logManager;
    MetricsManager* metricsManager;
    ThreadManager* threadManager;
    ScriptManager* scriptManager;
    IntelligenceManager* aiManager;

    Server* server;

    static std::shared_ptr<Application> theApp;
};
