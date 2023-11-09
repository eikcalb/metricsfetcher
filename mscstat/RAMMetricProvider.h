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

    virtual std::string GetName() { return  "RAMMetricProvider"; }

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


