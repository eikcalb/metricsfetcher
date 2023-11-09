#pragma once
#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "LogManager.h"
#include "MetricProviderBase.h"

class MetricsManager {
public:
    // Function to get the singleton instance
    static MetricsManager& GetInstance(int intervalMS) {
        static MetricsManager instance(intervalMS); // This ensures a single instance is created
        return instance;
    }

    // Start collecting metrics at a specified interval
    void StartMetricsCollection();

    // Stop collecting metrics
    void StopMetricsCollection() {
        isCollectingMetrics_ = false;
    }

    // Add metric providers to the manager
    void AddMetricProvider(std::unique_ptr<MetricProviderBase> provider) {
        metricProviders_.push_back(std::move(provider));
    }

    std::string GetInfoAsJSON() {
        const auto metricsActive = IsActive();
        const auto providers = GetActiveProviders();

        // Create a RapidJSON Document
        rapidjson::Document doc;
        doc.SetObject();

        rapidjson::Value isActive_;
        isActive_.SetBool(metricsActive);
        doc.AddMember("isActive", metricsActive, doc.GetAllocator());

        rapidjson::Value jsonArray(rapidjson::kArrayType);
        for (const auto& provider : providers) {

            rapidjson::Value name_;
            name_.SetString(provider.c_str(), doc.GetAllocator());

            jsonArray.PushBack(name_, doc.GetAllocator());
        }

        doc.AddMember("providers", jsonArray, doc.GetAllocator());

        // Serialize the Document to a JSON string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        buffer.Flush();
        doc.Accept(writer);

        return buffer.GetString();
    }

private:
    bool IsActive() { return isCollectingMetrics_; }

    std::vector<std::string> GetActiveProviders() {
        std::vector<std::string> providerNames;
        for (const auto& provider : metricProviders_) {
            providerNames.push_back(provider->GetName());
        }

        return providerNames;
    }

private:
    MetricsManager(int intervalMS) : intervalMS_(intervalMS) {} // Private constructor to prevent external instantiation
    std::vector<std::unique_ptr<MetricProviderBase>> metricProviders_;
    bool isCollectingMetrics_ = false; // Flag to control metrics collection
    int intervalMS_;
};