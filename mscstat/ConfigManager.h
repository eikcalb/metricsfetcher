#pragma once
#include <iostream>
#include <string>
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#ifndef METRICS_FETCHER_PORT
#define METRICS_FETCHER_PORT "METRICS_FETCHER_PORT"
#endif // !METRICS_FETCHER_PORT


struct MyConfig {
    // Default pool size is 4.
    short poolSize = 4;
    // Environmental variable overrides whatever is set for this value,
    // allowing the operating system to manage the port allocation and provide
    // an available port to the application.
    short port = (std::getenv(METRICS_FETCHER_PORT) != nullptr) ? std::atoi(std::getenv(METRICS_FETCHER_PORT)) : 8080;
    // Default interval for fetching each metric on a computer is 10 seconds
    short metricFetchInterval = 10 * 1000;
    // Default prediction interval is set to 5 minutes
    short predictionInterval = 5 * 60 * 1000;
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
        config = ParseJSONString(jsonString);

        std::cout << "Configuration loaded." << std::endl;
    }

    MyConfig GetConfig() const {
        std::lock_guard<std::mutex> lock(mutex);
        return config;
    }

    std::string GetConfigAsJSON() const {
        // Create a RapidJSON Document
        rapidjson::Document doc;
        doc.SetObject();

        doc.AddMember("poolSize", config.poolSize, doc.GetAllocator());
        doc.AddMember("port", config.port, doc.GetAllocator());
        doc.AddMember("metricFetchInterval", config.metricFetchInterval, doc.GetAllocator());
        doc.AddMember("predictionInterval", config.predictionInterval, doc.GetAllocator());

        // Serialize the Document to a JSON string
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        buffer.Flush();
        doc.Accept(writer);

        return buffer.GetString();
    }

    MyConfig ParseJSONString(const std::string& jsonString) {
        MyConfig config;
        rapidjson::Document document;

        if (jsonString.empty()) {
            return config;
        }

        document.Parse(jsonString.c_str());

        if (!document.IsObject() || document.HasParseError()) {
            std::cerr << "Invalid JSON format." << document.GetParseError() << std::endl;
            throw std::runtime_error("JSON is in wrong format." + document.HasParseError() ? " " + document.GetParseError() : "");
        }

        if (document.HasMember("poolSize") && document["poolSize"].IsUint()) {
            config.poolSize = document["poolSize"].GetInt();
        }
        if (document.HasMember("port") && document["port"].IsUint()) {
            config.port = document["port"].GetUint();
        }
        if (document.HasMember("metricFetchInterval") && document["metricFetchInterval"].IsUint()) {
            config.metricFetchInterval = document["metricFetchInterval"].GetUint();
        }
        if (document.HasMember("predictionInterval") && document["predictionInterval"].IsUint()) {
            config.predictionInterval = document["predictionInterval"].GetUint();
        }

        return config;
    }

private:
    ConfigManager() {}
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    MyConfig config;
    mutable std::mutex mutex;
};

