#include "MetricsManager.h"
#include "Application.h"

// Start collecting metrics at a specified interval
void MetricsManager::StartMetricsCollection() {
    // Set the flag to indicate that metrics collection is active
    isCollectingMetrics_ = true;
    counter = 0;

    // Start a loop to collect metrics at the specified interval.
    // In order to get conformity and track each metric fetch cycle, we will pass the counter
    // value to metrics providers. This way, each metric recorded will have insight to when the
    // data was fetched relative to other metric providers.
    while (isCollectingMetrics_.load()) {
        for (const auto& provider : metricProviders_) {
            Application::theApp->threadManager->AddTaskToThread([&] {
                provider->RetrieveMetricValue(counter.load());
                });
        }
        Application::theApp->scriptManager->Process(counter);

        counter++;

        // Minimum interval is 1 second
        const auto interval = intervalMS_ < 4000 ? 4000 : intervalMS_;
        LogManager::GetInstance().LogInfo(
            "Metrics Fetched: {0}. Next fetch in {1} seconds. ",
            counter.load(),
            interval / 1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
}

std::string MetricsManager::GetProviderDataJSON(const UINT8 count) const {
    const auto nextUpdateTime = Application::theApp->configManager->GetConfig().metricFetchInterval;

    // Create a RapidJSON Document
    rapidjson::Document doc;
    doc.SetObject();

    rapidjson::Value nextUpdate;
    nextUpdate.SetInt(nextUpdateTime);
    doc.AddMember("nextUpdateTime", nextUpdateTime, doc.GetAllocator());

    // Here we will fetch the number of data specified in `count`.
    rapidjson::Value jsonArray(rapidjson::kArrayType);
    for (const auto& provider : metricProviders_) {
        rapidjson::Value obj(rapidjson::kObjectType);

        rapidjson::Value name;
        name.SetString(provider->GetName().c_str(), doc.GetAllocator());
        obj.AddMember("name", name, doc.GetAllocator());
        obj.AddMember("data", provider->GetDataJSON(doc, count), doc.GetAllocator());

        rapidjson::Value isCustom;
        isCustom.SetBool(false);
        obj.AddMember("isCustom", isCustom, doc.GetAllocator());
        jsonArray.PushBack(obj, doc.GetAllocator());
    }

    // Populate custom metrics.
    Application::theApp->scriptManager->updateDataJSONArray(doc, jsonArray, count);

    doc.AddMember("providers", jsonArray, doc.GetAllocator());

    // Serialize the Document to a JSON string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    buffer.Flush();
    doc.Accept(writer);

    return buffer.GetString();
}

std::string MetricsManager::GetProviderAggregateDataJSON(const std::string column, const bool isCustom, const std::string name) const {
    // Create a RapidJSON Document
    rapidjson::Document doc;
    doc.SetObject();

    // Find the provider with the name
    if (isCustom) {
        doc.AddMember("data", Application::theApp->scriptManager->GetAggregateDataJSON(doc, column), doc.GetAllocator());

        // Serialize the Document to a JSON string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        buffer.Flush();
        doc.Accept(writer);

        return buffer.GetString();
    }
    else {
        for (auto& p : metricProviders_) {
            if (p->GetName() == name) {
                doc.AddMember("data", p->GetAggregateDataJSON(doc, column), doc.GetAllocator());

                // Serialize the Document to a JSON string
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                buffer.Flush();
                doc.Accept(writer);

                return buffer.GetString();
            }

        }
    }
    throw std::runtime_error("Provider with specified name not found");
}
