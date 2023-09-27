#pragma once
#include <iostream>
#include <string>
#include <mutex>
#include <rapidjson/document.h>

struct MyConfig {
    // Default pool size is 2.
    int poolSize = 2;
    // Default interval for fetching each metric on a computer is 10 seconds
    int metricFetchInterval = 10 * 10;
};

class ConfigManager {
public:
    static ConfigManager& GetInstance() {
        static ConfigManager instance;
        return instance;
    }

    void LoadConfig(const std::string& jsonString) {
        if (jsonString.empty()) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex);

        rapidjson::Document document;
        document.Parse(jsonString.c_str());

        if (!document.IsObject()) {
            std::cerr << "Invalid JSON format." << std::endl;
            return;
        }

        if (document.HasMember("poolSize") && document["poolSize"].IsInt()) {
            config.poolSize = document["poolSize"].GetInt();
        }
        if (document.HasMember("metricFetchInterval") && document["metricFetchInterval"].IsInt()) {
            config.metricFetchInterval = document["metricFetchInterval"].GetInt();
        }
        std::cout << "Configuration loaded." << std::endl;
    }

    MyConfig GetConfig() const {
        std::lock_guard<std::mutex> lock(mutex);
        return config;
    }

private:
    ConfigManager() {}
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    MyConfig config;
    mutable std::mutex mutex;
};

