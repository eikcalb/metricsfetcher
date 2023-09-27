#pragma once
#include "MetricProviderBase.h"

class ProcessMetricProvider: public MetricProviderBase {
    struct Metric {
        std::string name;
        UINT16 counter;
        double read = 0;
        double write = 0;
        double transferRate = 0;
    };

    public:
        ProcessMetricProvider() {
            // Create a table for this metric provider if it does not exist
            DataManager::GetInstance().CreateTable("ProcessMetricProvider", " \
            id INTEGER PRIMARY KEY, \
            name TEXT DEFAULT \"Process\", \
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

        virtual void RetrieveMetricValue(UINT16 counter) override {
            // Save the data to the database
            latestValue = std::make_shared<Metric>();

            latestValue->name = "Storage";
            latestValue->counter = counter;
            latestValue->read = GetDiskReadRate();
            latestValue->write = GetDiskWriteRate();
            latestValue->transferRate = GetTotalTransferRate();

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

            DataManager::GetInstance().Insert("ProcessMetricProvider", sqlString);
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

