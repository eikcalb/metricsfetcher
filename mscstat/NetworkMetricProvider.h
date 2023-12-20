#pragma once
#include <locale>
#include <codecvt>

#include "MetricProviderBase.h"
#include "Utils.h"

// All units are in bytes/sec
class NetworkMetricProvider : public MetricProviderBase {
    struct Metric {
        std::string name;
        UINT16 counter = 0;
        double bytesSentPerSecond = 0;      // bytes/sec
        double bytesReceivedPerSecond = 0;  // bytes/sec
        double bytesTotalPerSecond = 0;     // bytes/sec
        double currentBandwidth = 0;        // bytes/sec
        double packetsReceivedPerSecond = 0; // packets/sec
        double packetsSentPerSecond = 0;     // packets/sec
        double connectionsActive = 0;        // count
        double connectionsEstablished = 0;   // count
        double networkErrorsPerSecond = 0;   // errors/sec
    };

public:
    NetworkMetricProvider(std::string interfaceName) {
        this->name = interfaceName;
        // Create a table for this metric provider if it does not exist
        DataManager::GetInstance().CreateTable("NetworkMetricProvider", " \
            id INTEGER PRIMARY KEY, \
            name TEXT DEFAULT \"" + interfaceName + "\", \
            counter INTEGER NOT NULL, \
            bytesSentPerSecond REAL DEFAULT 0, \
            bytesReceivedPerSecond REAL DEFAULT 0, \
            bytesTotalPerSecond REAL DEFAULT 0, \
            currentBandwidth REAL DEFAULT 0, \
            packetsReceivedPerSecond REAL DEFAULT 0, \
            packetsSentPerSecond REAL DEFAULT 0, \
            connectionsActive REAL DEFAULT 0, \
            connectionsEstablished REAL DEFAULT 0, \
            networkErrorsPerSecond REAL DEFAULT 0, \
            timestamp INTEGER NOT NULL");

        PDH_STATUS status = PdhOpenQuery(nullptr, 0, &queryHandle);
        if (status != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to open PDH query.");
        }

        // Create a wstring using wstring_convert
        std::wstring wideString = Utils::StringToWstring(name);

        std::wstring bytesSentName = L"\\Network Interface(" + wideString + L")\\Bytes Sent/sec";
        std::wstring bytesRecivedName = L"\\Network Interface(" + wideString + L")\\Bytes Received/sec";
        std::wstring bytesTotalName = L"\\Network Interface(" + wideString + L")\\Bytes Total/sec";
        std::wstring currentBandwidthName = L"\\Network Interface(" + wideString + L")\\Current Bandwidth";
        std::wstring packetsRecivedName = L"\\Network Interface(" + wideString + L")\\Packets Received/sec";
        std::wstring packetsSentName = L"\\Network Interface(" + wideString + L")\\Packets Sent/sec";
        std::wstring connectionsActiveName = L"\\Network Interface(" + wideString + L")\\Connections Active";
        std::wstring connectionsEstablishedName = L"\\Network Interface(" + wideString + L")\\Connections Established";
        std::wstring networkErrorName = L"\\Network Interface(" + wideString + L")\\Network Error/sec";

        PdhAddEnglishCounter(queryHandle, bytesSentName.c_str(), 0, &bytesSentCounter);
        PdhAddEnglishCounter(queryHandle, bytesRecivedName.c_str(), 0, &bytesReceivedCounter);
        PdhAddEnglishCounter(queryHandle, bytesTotalName.c_str(), 0, &bytesTotalCounter);
        PdhAddEnglishCounter(queryHandle, currentBandwidthName.c_str(), 0, &currentBandwidthCounter);
        PdhAddEnglishCounter(queryHandle, packetsRecivedName.c_str(), 0, &packetsReceivedCounter);
        PdhAddEnglishCounter(queryHandle, packetsSentName.c_str(), 0, &packetsSentCounter);
        PdhAddEnglishCounter(queryHandle, connectionsActiveName.c_str(), 0, &connectionsActiveCounter);
        PdhAddEnglishCounter(queryHandle, connectionsEstablishedName.c_str(), 0, &connectionsEstablishedCounter);
        PdhAddEnglishCounter(queryHandle, networkErrorName.c_str(), 0, &networkErrorsCounter);

        PdhCollectQueryData(queryHandle);
    }

    virtual rapidjson::Value GetDataJSON(rapidjson::Document& doc, const UINT8 count) const {
        // Fetch the most recent data, up to `count`
        rapidjson::Value response(rapidjson::kArrayType);

        const auto rows = DataManager::GetInstance().Select("NetworkMetricProvider",
            "name = \"" + name + "\"  ORDER BY id DESC LIMIT " + std::to_string(count));
        for (auto& row : rows) {
            rapidjson::Value obj(rapidjson::kObjectType);

            auto id = row.GetInt("id");
            auto counter = row.GetInt("counter");
            auto timestamp = row.GetInt("timestamp");

            auto bytesSentPerSecond = row.GetDouble("bytesSentPerSecond");
            auto bytesReceivedPerSecond = row.GetDouble("bytesReceivedPerSecond");
            auto bytesTotalPerSecond = row.GetDouble("bytesTotalPerSecond");
            auto currentBandwidth = row.GetDouble("currentBandwidth");
            auto packetsReceivedPerSecond = row.GetDouble("packetsReceivedPerSecond");
            auto packetsSentPerSecond = row.GetDouble("packetsSentPerSecond");
            auto connectionsActive = row.GetDouble("connectionsActive");
            auto connectionsEstablished = row.GetDouble("connectionsEstablished");
            auto networkErrorsPerSecond = row.GetDouble("networkErrorsPerSecond");


            obj.AddMember("id", Utils::ConvertIntToJSONValue(id, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("counter", Utils::ConvertIntToJSONValue(counter, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("timestamp", Utils::ConvertIntToJSONValue(timestamp, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("bytesSentPerSecond", Utils::ConvertDoubleToJSONValue(bytesSentPerSecond, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("bytesReceivedPerSecond", Utils::ConvertDoubleToJSONValue(bytesReceivedPerSecond, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("bytesTotalPerSecond", Utils::ConvertDoubleToJSONValue(bytesTotalPerSecond, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("currentBandwidth", Utils::ConvertDoubleToJSONValue(currentBandwidth, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("packetsReceivedPerSecond", Utils::ConvertDoubleToJSONValue(packetsReceivedPerSecond, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("packetsSentPerSecond", Utils::ConvertDoubleToJSONValue(packetsSentPerSecond, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("connectionsActive", Utils::ConvertDoubleToJSONValue(connectionsActive, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("connectionsEstablished", Utils::ConvertDoubleToJSONValue(connectionsEstablished, doc.GetAllocator()), doc.GetAllocator());
            obj.AddMember("networkErrorsPerSecond", Utils::ConvertDoubleToJSONValue(networkErrorsPerSecond, doc.GetAllocator()), doc.GetAllocator());

            response.PushBack(obj, doc.GetAllocator());
        }

        return response;
    };

    virtual rapidjson::Value GetAggregateDataJSON(rapidjson::Document& doc, const std::string column) const {
        const auto& rows = DataManager::GetInstance().SelectAggregate("NetworkMetricProvider", column, "name = \"" + name + "\"");
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

    virtual std::string GetName() { return  name; }
    virtual bool IsMulti() { return  true; }

    virtual void RetrieveMetricValue(UINT16 counter) override {
        // Save the data to the database
        latestValue = std::make_shared<Metric>();
        latestValue->name = name;
        latestValue->counter = counter;

        if (CollectData()) {
            latestValue->bytesSentPerSecond = GetCounterValue(bytesSentCounter);
            latestValue->bytesReceivedPerSecond = GetCounterValue(bytesReceivedCounter);
            latestValue->bytesTotalPerSecond = GetCounterValue(bytesTotalCounter);
            latestValue->currentBandwidth = GetCounterValue(currentBandwidthCounter);
            latestValue->packetsReceivedPerSecond = GetCounterValue(packetsReceivedCounter);
            latestValue->packetsSentPerSecond = GetCounterValue(packetsSentCounter);
            latestValue->connectionsActive = GetCounterValue(connectionsActiveCounter);
            latestValue->connectionsEstablished = GetCounterValue(connectionsEstablishedCounter);
            latestValue->networkErrorsPerSecond = GetCounterValue(networkErrorsCounter);
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
            << latestValue->bytesSentPerSecond << ", "
            << latestValue->bytesReceivedPerSecond << ", "
            << latestValue->bytesTotalPerSecond << ", "
            << latestValue->currentBandwidth << ", "
            << latestValue->packetsReceivedPerSecond << ", "
            << latestValue->packetsSentPerSecond << ", "
            << latestValue->connectionsActive << ", "
            << latestValue->connectionsEstablished << ", "
            << latestValue->networkErrorsPerSecond << ", "
            << std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
        std::string sqlString = stream.str();

        DataManager::GetInstance().Insert("NetworkMetricProvider", sqlString);
    };

private:
    PDH_HCOUNTER bytesSentCounter;
    PDH_HCOUNTER bytesReceivedCounter;
    PDH_HCOUNTER bytesTotalCounter;
    PDH_HCOUNTER currentBandwidthCounter;
    PDH_HCOUNTER packetsReceivedCounter;
    PDH_HCOUNTER packetsSentCounter;
    PDH_HCOUNTER connectionsActiveCounter;
    PDH_HCOUNTER connectionsEstablishedCounter;
    PDH_HCOUNTER networkErrorsCounter;

    std::shared_ptr<Metric> latestValue = NULL;
    std::string name;
};
