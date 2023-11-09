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
            processCount INTEGER DEFAULT 0, \
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

        virtual std::string GetName() { return  "ProcessMetricProvider"; }

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

