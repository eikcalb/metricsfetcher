#pragma once
#include "MetricProviderBase.h"

// All units are in bytes/sec
class StorageMetricProvider : public MetricProviderBase {
    struct Metric {
        std::string name;
        UINT16 counter;
        double read = 0;
        double write = 0;
        double transferRate = 0;
    };

public:
    StorageMetricProvider() {
        // Create a table for this metric provider if it does not exist
        DataManager::GetInstance().CreateTable("StorageMetricProvider", " \
            id INTEGER PRIMARY KEY, \
            name TEXT DEFAULT \"Storage\", \
            counter INTEGER NOT NULL, \
            read REAL DEFAULT 0, \
            write REAL DEFAULT 0, \
            transferRate REAL DEFAULT 0, \
            timestamp INTEGER NOT NULL");

        PDH_STATUS status = PdhOpenQuery(nullptr, 0, &queryHandle);
        if (status != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to open PDH query.");
        }

        PdhAddEnglishCounter(queryHandle, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &diskReadRateCounter);
        PdhAddEnglishCounter(queryHandle, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &diskWriteRateCounter);
        PdhAddEnglishCounter(queryHandle, L"\\PhysicalDisk(_Total)\\Disk Bytes/sec", 0, &totalTransferRateCounter);

        PdhCollectQueryData(queryHandle);
    }


    virtual rapidjson::Value GetDataJSON(rapidjson::Document& doc, const UINT8 count) const {
        // Fetch the most recent data, up to `count`
        rapidjson::Value response(rapidjson::kArrayType);

        const auto rows = DataManager::GetInstance().Select("StorageMetricProvider", "1=1 ORDER BY id DESC LIMIT " + std::to_string(count));
        for (auto& row : rows) {
            rapidjson::Value obj(rapidjson::kObjectType);

            auto id = row.GetInt("id");
            auto counter = row.GetInt("counter");
            auto timestamp = row.GetInt("timestamp");

            auto read = row.GetDouble("read");
            auto write = row.GetDouble("write");
            auto transferRate = row.GetDouble("transferRate");

            obj.AddMember("id", Utils::ConvertIntToJSONValue(id, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("counter", Utils::ConvertIntToJSONValue(counter, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("timestamp", Utils::ConvertIntToJSONValue(timestamp, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("read", Utils::ConvertDoubleToJSONValue(read, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("write", Utils::ConvertDoubleToJSONValue(write, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("transferRate", Utils::ConvertDoubleToJSONValue(transferRate, doc.GetAllocator()), doc.GetAllocator());

            response.PushBack(obj, doc.GetAllocator());
        }

        return response;
    };

    virtual rapidjson::Value GetAggregateDataJSON(rapidjson::Document& doc, const std::string column) const {
        const auto& rows = DataManager::GetInstance().SelectAggregate("StorageMetricProvider", column);
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

    virtual std::string GetName() { return  "Storage"; }

    virtual void RetrieveMetricValue(UINT16 counter) override {
        // Save the data to the database
        latestValue = std::make_shared<Metric>();
        latestValue->name = "Storage";
        latestValue->counter = counter;

        if (CollectData()) {
            latestValue->read = GetDiskReadRate();
            latestValue->write = GetDiskWriteRate();
            latestValue->transferRate = GetTotalTransferRate();
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
            << latestValue->read << ", "
            << latestValue->write << ", "
            << latestValue->transferRate << ", "
            << std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
        std::string sqlString = stream.str();

        DataManager::GetInstance().Insert("StorageMetricProvider", sqlString);
    };

private:
    double GetDiskReadRate() {
        return GetCounterValue(diskReadRateCounter);
    }

    double GetDiskWriteRate() {
        return GetCounterValue(diskWriteRateCounter);
    }

    double GetTotalTransferRate() {
        return GetCounterValue(totalTransferRateCounter);
    }

private:
    PDH_HCOUNTER diskReadRateCounter;
    PDH_HCOUNTER diskWriteRateCounter;
    PDH_HCOUNTER totalTransferRateCounter;

    std::shared_ptr<Metric> latestValue = NULL;
};

