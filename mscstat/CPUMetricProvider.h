#pragma once
#include "MetricProviderBase.h"

class CPUMetricProvider : public MetricProviderBase {
	struct Metric {
		std::string name;
		UINT16 counter;
		double usage = 0;
		double instructionsRetired = 0;
		double cycles = 0;
		double floatingPointOperations = 0;
		double temperature = 0;
	};

public:
	CPUMetricProvider() {
		// Create a table for this metric provider if it does not exist
		DataManager::GetInstance().CreateTable("CPUMetricProvider", " \
            id INTEGER PRIMARY KEY, \
            name TEXT DEFAULT \"CPU\", \
            counter INTEGER NOT NULL, \
            usage REAL DEFAULT 0, \
            instructionsRetired REAL DEFAULT 0, \
            cycles REAL DEFAULT 0, \
            floatingPointOperations REAL DEFAULT 0, \
            temperature REAL DEFAULT 0, \
            timestamp INTEGER NOT NULL");

		PDH_STATUS status = PdhOpenQuery(nullptr, 0, &queryHandle);
		if (status != ERROR_SUCCESS) {
			throw std::runtime_error("Failed to open PDH query.");
		}

		PdhAddEnglishCounter(queryHandle, L"\\Processor(_Total)\\% Processor Time", 0, &cpuUsageCounter);
		PdhAddCounter(queryHandle, L"\\Processor(_Total)\\Instructions Retired", 0, &instructionsRetiredCounter);
		PdhAddCounter(queryHandle, L"\\Processor(_Total)\\Cycles", 0, &cyclesCounter);
		PdhAddCounter(queryHandle, L"\\Processor(_Total)\\Floating Point Operations/sec", 0, &floatingPointOperationsCounter);
		PdhAddCounter(queryHandle, L"\\Thermal Zone Information\\_TZ.Temperature", 0, &temperatureCounter);

		PdhCollectQueryData(queryHandle);
	}

    virtual std::string GetName() { return  "CPUMetricProvider"; }

	virtual void RetrieveMetricValue(UINT16 counter) override {
		// Save the data to the database
		latestValue = std::make_shared<Metric>();
		latestValue->name = "CPU";
		latestValue->counter = counter;

		if (CollectData()) {
			latestValue->cycles = GetCycles();
			latestValue->instructionsRetired = GetInstructionsRetired();
			latestValue->temperature = GetTemperature();
			latestValue->usage = GetUsage();
			latestValue->floatingPointOperations = GetFloatingPointOperations();
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
			<< latestValue->usage << ", "
			<< latestValue->instructionsRetired << ", "
			<< latestValue->cycles << ", "
			<< latestValue->floatingPointOperations << ", "
			<< latestValue->temperature << ", "
			<< std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
		std::string sqlString = stream.str();

		DataManager::GetInstance().Insert("CPUMetricProvider", sqlString);
	};

private:
	double GetUsage() {
		return GetCounterValue(cpuUsageCounter);
	}

	double GetInstructionsRetired() {
		return GetCounterValue(instructionsRetiredCounter);
	}

	double GetCycles() {
		return GetCounterValue(cyclesCounter);
	}

	double GetFloatingPointOperations() {
		return GetCounterValue(floatingPointOperationsCounter);
	}

	double GetTemperature() {
		return GetCounterValue(temperatureCounter);
	}

private:
	PDH_HCOUNTER cpuUsageCounter;
	PDH_HCOUNTER instructionsRetiredCounter;
	PDH_HCOUNTER cyclesCounter;
	PDH_HCOUNTER floatingPointOperationsCounter;
	PDH_HCOUNTER temperatureCounter;

	std::shared_ptr<Metric> latestValue = NULL;
};

