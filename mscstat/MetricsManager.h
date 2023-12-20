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
        isCollectingMetrics_.store(false);
    }

    // Add metric providers to the manager
    void AddMetricProvider(std::unique_ptr<MetricProviderBase> provider) {
        metricProviders_.emplace_back(std::move(provider));
    }

    std::string GetInfoAsJSON() const {
        const auto metricsActive = IsActive();
        const auto providers = GetActiveProviders();

        // Create a RapidJSON Document
        rapidjson::Document doc;
        doc.SetObject();

        rapidjson::Value isActive_;
        isActive_.SetBool(metricsActive);
        doc.AddMember("isActive", isActive_, doc.GetAllocator());

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

    std::string GetProviderDataJSON(const UINT8 count) const;

    std::string GetProviderAggregateDataJSON(const std::string column, const bool isCustom, const std::string name = "") const;

    std::string GetAvailableCountersJSON() {
        // Create a RapidJSON Document
        rapidjson::Document doc;
        doc.SetObject();

        rapidjson::Value jsonArray(rapidjson::kArrayType);
        for (const auto& counter : *availableCounters) {
            rapidjson::Value name_;
            name_.SetString(counter.c_str(), doc.GetAllocator());

            jsonArray.PushBack(name_, doc.GetAllocator());
        }

        doc.AddMember("counters", jsonArray, doc.GetAllocator());

        // Serialize the Document to a JSON string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        buffer.Flush();
        doc.Accept(writer);

        return buffer.GetString();
    }

private:
    bool IsActive() const { return isCollectingMetrics_.load(); }

    std::vector<std::string> GetActiveProviders() const {
        std::vector<std::string> providerNames;
        for (const auto& provider : metricProviders_) {
            providerNames.push_back(provider->GetName());
        }

        return providerNames;
    }

private:
    MetricsManager(int intervalMS) : intervalMS_(intervalMS) {
        // Here we will get the list of available counters on the computer
        availableCounters = std::make_shared<std::vector<std::string>>(Utils::ExecuteShellCommand("typeperf -qx"));
    } // Private constructor to prevent external instantiation
    std::vector<std::unique_ptr<MetricProviderBase>> metricProviders_;
    std::atomic<bool> isCollectingMetrics_ = false; // Flag to control metrics collection
    std::atomic<UINT64> counter = 0;
    int intervalMS_;

    std::shared_ptr<std::vector<std::string>> availableCounters;
};