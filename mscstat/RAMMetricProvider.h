#pragma once
#include "MetricProviderBase.h"

// All units are in bytes/sec
class RAMMetricProvider : public MetricProviderBase {
    struct Metric {
        std::string name;
        UINT16 counter;
        double available = 0; // bytes
        double committed = 0; // bytes
        double pageFaults = 0; // fault/sec
    };

public:
    RAMMetricProvider() {
        // Create a table for this metric provider if it does not exist
        DataManager::GetInstance().CreateTable("RAMMetricProvider", " \
            id INTEGER PRIMARY KEY, \
            name TEXT DEFAULT \"Memory\", \
            counter INTEGER NOT NULL, \
            available REAL DEFAULT 0, \
            committed REAL DEFAULT 0, \
            pageFaults REAL DEFAULT 0, \
            timestamp INTEGER NOT NULL");

        PDH_STATUS status = PdhOpenQuery(nullptr, 0, &queryHandle);
        if (status != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to open PDH query.");
        }

        PdhAddEnglishCounter(queryHandle, L"\\Memory\\Available Bytes", 0, &availableCounter);
        PdhAddEnglishCounter(queryHandle, L"\\Memory\\Committed Bytes", 0, &committedCounter);
        PdhAddEnglishCounter(queryHandle, L"\\Memory\\Page Faults/sec", 0, &pageFaultsCounter);

        PdhCollectQueryData(queryHandle);
    }

    virtual rapidjson::Value GetDataJSON(rapidjson::Document& doc, const UINT8 count) const {
        // Fetch the most recent data, up to `count`
        rapidjson::Value response(rapidjson::kArrayType);

        const auto rows = DataManager::GetInstance().Select("RAMMetricProvider", "1=1 ORDER BY id DESC LIMIT " + std::to_string(count));
        for (auto& row : rows) {
            rapidjson::Value obj(rapidjson::kObjectType);

            auto id = row.GetInt("id");
            auto counter = row.GetInt("counter");
            auto timestamp = row.GetInt("timestamp");

            auto available = row.GetDouble("available");
            auto committed = row.GetDouble("committed");
            auto pageFaults = row.GetDouble("pageFaults");

            obj.AddMember("id", Utils::ConvertIntToJSONValue(id, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("counter", Utils::ConvertIntToJSONValue(counter, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("timestamp", Utils::ConvertIntToJSONValue(timestamp, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("available", Utils::ConvertDoubleToJSONValue(available, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("committed", Utils::ConvertDoubleToJSONValue(committed, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("pageFaults", Utils::ConvertDoubleToJSONValue(pageFaults, doc.GetAllocator()), doc.GetAllocator());

            response.PushBack(obj, doc.GetAllocator());
        }

        return response;
    };

    virtual rapidjson::Value GetAggregateDataJSON(rapidjson::Document& doc, const std::string column) const {
        const auto& rows = DataManager::GetInstance().SelectAggregate("RAMMetricProvider", column);
        rapidjson::Value obj(rapidjson::kObjectType);

        const auto& row = rows[0];
        auto max = row.GetDouble("max");
        auto min = row.GetDouble("min");
        auto avg = row.GetDouble("avg");
        auto total = row.GetDouble("total");
        auto count = row.GetInt("count");

        obj.AddMember("max", Utils::ConvertDoubleToJSONValue(max, doc.GetAllocator()), doc.GetAllocator());
        obj.AddMember("min", Utils::ConvertDoubleToJSONValue(min, doc.GetAllocator()), doc.GetAllocator());
        obj.AddMember("avg", Utils::ConvertDoubleToJSONValue(avg, doc.GetAllocator()), doc.GetAllocator());
        obj.AddMember("total", Utils::ConvertDoubleToJSONValue(total, doc.GetAllocator()), doc.GetAllocator());
        obj.AddMember("count", Utils::ConvertDoubleToJSONValue(count, doc.GetAllocator()), doc.GetAllocator());

        return obj;
    };

    virtual std::string GetName() { return  "Memory"; }

    virtual void RetrieveMetricValue(UINT16 counter) override {
        // Save the data to the database
        latestValue = std::make_shared<Metric>();
        latestValue->name = "Memory";
        latestValue->counter = counter;

        if (CollectData()) {
            latestValue->available = GetAvailableRate();
            latestValue->committed = GetCommittedRate();
            latestValue->pageFaults = GetPageFaultsRate();
        }

        Persist();
    }

protected:
    virtual void Persist() override {
        const auto p1 = std::chrono::system_clock::now();

        // Create a stringstream object
        std::ostringstream stream{};
        stream << "NULL, "
            << "\"" << latestValue->name << "\", "
            << latestValue->counter << ", "
            << latestValue->available << ", "
            << latestValue->committed << ", "
            << latestValue->pageFaults << ", "
            << std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
        std::string sqlString = stream.str();

        DataManager::GetInstance().Insert("RAMMetricProvider", sqlString);
    };

private:
    double GetAvailableRate() {
        return GetCounterValue(availableCounter);
    }

    double GetCommittedRate() {
        return GetCounterValue(committedCounter);
    }

    double GetPageFaultsRate() {
        return GetCounterValue(pageFaultsCounter);
    }

private:
    PDH_HCOUNTER availableCounter;
    PDH_HCOUNTER committedCounter;
    PDH_HCOUNTER pageFaultsCounter;

    std::shared_ptr<Metric> latestValue = NULL;
};


