#pragma once
#include "MetricProviderBase.h"

class ProcessMetricProvider: public MetricProviderBase {
    struct Metric {
        std::string name;
        UINT16 counter;
        double read = 0;
        double write = 0;
        double processCount = 0;
        std::string activeProcess= "";
        std::string activeWindow = "";
    };

    public:
        ProcessMetricProvider() {
            // Create a table for this metric provider if it does not exist
            DataManager::GetInstance().CreateTable("ProcessMetricProvider", " \
            id INTEGER PRIMARY KEY, \
            name TEXT DEFAULT \"Process\", \
            counter INTEGER NOT NULL, \
            processCount REAL DEFAULT 0, \
            activeProcess TEXT DEFAULT \"\", \
            activeWindow TEXT DEFAULT \"\", \
            bytesReadPerSecond REAL DEFAULT 0, \
            bytesWrittenPerSecond REAL DEFAULT 0, \
            timestamp INTEGER NOT NULL");

            PDH_STATUS status = PdhOpenQuery(nullptr, 0, &queryHandle);
            if (status != ERROR_SUCCESS) {
                throw std::runtime_error("Failed to open PDH query.");
            }

            PdhAddEnglishCounter(queryHandle, L"\\System\\Processes", 0, &processCounter);
            PdhAddEnglishCounter(queryHandle, L"\\Process(_Total)\\IO Read Bytes/sec", 0, &readRateCounter);
            PdhAddEnglishCounter(queryHandle, L"\\Process(_Total)\\IO Write Bytes/sec", 0, &writeRateCounter);


            PdhCollectQueryData(queryHandle);
        }

        virtual rapidjson::Value GetDataJSON(rapidjson::Document& doc, const UINT8 count) const {
            // Fetch the most recent data, up to `count`
            rapidjson::Value response(rapidjson::kArrayType);

            const auto rows = DataManager::GetInstance().Select("ProcessMetricProvider", "1=1 ORDER BY id DESC LIMIT " + std::to_string(count));
            for (auto& row : rows) {
                rapidjson::Value obj(rapidjson::kObjectType);

                auto id = row.GetInt("id");
                auto counter = row.GetInt("counter");
                auto timestamp = row.GetInt("timestamp");

                auto processCount = row.GetDouble("processCount");
                auto activeProcess = row.GetString("activeProcess");
                auto activeWindow = row.GetString("activeWindow");
                auto bytesReadPerSecond = row.GetDouble("bytesReadPerSecond");
                auto bytesWrittenPerSecond = row.GetDouble("bytesWrittenPerSecond");

                obj.AddMember("id", Utils::ConvertIntToJSONValue(id, doc.GetAllocator()), doc.GetAllocator());
                obj.AddMember("counter", Utils::ConvertIntToJSONValue(counter, doc.GetAllocator()), doc.GetAllocator());
                obj.AddMember("timestamp", Utils::ConvertIntToJSONValue(timestamp, doc.GetAllocator()), doc.GetAllocator());
                obj.AddMember("processCount", Utils::ConvertIntToJSONValue(processCount, doc.GetAllocator()), doc.GetAllocator());
                obj.AddMember("bytesReadPerSecond", Utils::ConvertDoubleToJSONValue(bytesReadPerSecond, doc.GetAllocator()), doc.GetAllocator());
                obj.AddMember("bytesWrittenPerSecond", Utils::ConvertDoubleToJSONValue(bytesWrittenPerSecond, doc.GetAllocator()), doc.GetAllocator());

                rapidjson::Value activeProcess_;
                activeProcess_.SetString(activeProcess.c_str(), doc.GetAllocator());
                obj.AddMember("activeProcess", activeProcess_, doc.GetAllocator());

                rapidjson::Value activeWindow_;
                activeProcess_.SetString(activeProcess.c_str(), doc.GetAllocator());
                obj.AddMember("activeWindow",activeWindow_, doc.GetAllocator());

                response.PushBack(obj, doc.GetAllocator());
            }

            return response;
        };

        virtual rapidjson::Value GetAggregateDataJSON(rapidjson::Document& doc, const std::string column) const {
            const auto& rows = DataManager::GetInstance().SelectAggregate("ProcessMetricProvider", column);
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

        virtual std::string GetName() { return  "Process"; }

        virtual void RetrieveMetricValue(UINT16 counter) override {
            // Save the data to the database
            latestValue = std::make_shared<Metric>();
            latestValue->name = "Process";
            latestValue->counter = counter;

            if (CollectData()) {
                latestValue->processCount = GetCounterValue(processCounter);
                latestValue->activeProcess= Utils::GetActiveProcessTitle();
                latestValue->activeWindow = Utils::GetActiveWindowTitle();
                latestValue->read = GetCounterValue(readRateCounter);
                latestValue->write = GetCounterValue(writeRateCounter);
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
                << latestValue->processCount << ", "
                << "\"" << latestValue->activeProcess<< "\"" << ", "
                << "\"" << latestValue->activeWindow << "\"" << ", "
                << latestValue->read << ", "
                << latestValue->write << ", "
                << std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
            std::string sqlString = stream.str();

            DataManager::GetInstance().Insert("ProcessMetricProvider", sqlString);
        };

    private:
        PDH_HCOUNTER processCounter;
        PDH_HCOUNTER readRateCounter;
        PDH_HCOUNTER writeRateCounter;

        std::shared_ptr<Metric> latestValue = NULL;
};

