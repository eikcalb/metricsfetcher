#include "MetricsManager.h"
#include "Application.h"

// Start collecting metrics at a specified interval
void MetricsManager::StartMetricsCollection() {
    // Set the flag to indicate that metrics collection is active
    isCollectingMetrics_ = true;

    static UINT64 counter = 0;

    // Start a loop to collect metrics at the specified interval.
    // In order to get conformity and track each metric fetch cycle, we will pass the counter
    // value to metrics providers. This way, each metric recorded will have insight to when the
    // data was fetched relative to other metric providers.
    while (isCollectingMetrics_) {
        //for (const auto& provider : metricProviders_) {
        //    Application::theApp->threadManager->AddTaskToThread([&] {
        //        provider->RetrieveMetricValue(counter);
        //    });
        //}
        Application::theApp->scriptManager->Process(counter);

        counter++;

        // Minimum interval is 1 second
        const auto interval = intervalMS_ < 1000 ? 1000 : intervalMS_;
        LogManager::GetInstance().LogInfo("Metrics Fetched: {0}. Next fetch in {1} seconds. ", counter, interval / 1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
}