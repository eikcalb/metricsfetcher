#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>

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
    void StartMetricsCollection() {
        // Set the flag to indicate that metrics collection is active
        isCollectingMetrics_ = true;

        static UINT64 counter = 0;

        // Start a loop to collect metrics at the specified interval
        while (isCollectingMetrics_) {
            for (const auto& provider : metricProviders_) {
                provider->RetrieveMetricValue(counter);
            }
            counter++;

            const auto interval = intervalMS_ < 1000 ? 1000 : intervalMS_;
            LogManager::GetInstance().LogInfo("Metrics Fetched: {0}. Next fetch in {1} seconds. ", counter, interval / 1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    }

    // Stop collecting metrics
    void StopMetricsCollection() {
        isCollectingMetrics_ = false;
    }

    // Add metric providers to the manager
    void AddMetricProvider(std::unique_ptr<MetricProviderBase> provider) {
        metricProviders_.push_back(std::move(provider));
    }

private:
    MetricsManager(int intervalMS): intervalMS_(intervalMS) {} // Private constructor to prevent external instantiation
    std::vector<std::unique_ptr<MetricProviderBase>> metricProviders_;
    bool isCollectingMetrics_ = false; // Flag to control metrics collection
    int intervalMS_;
};