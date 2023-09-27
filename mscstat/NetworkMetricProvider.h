#pragma once
#include "MetricProviderBase.h"

// All units are in bytes/sec
class NetworkMetricProvider : public MetricProviderBase {
	struct Metric {
		std::string name;
		UINT16 counter;
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
	NetworkMetricProvider() {
		// Create a table for this metric provider if it does not exist
		DataManager::GetInstance().CreateTable("NetworkMetricProvider", " \
            id INTEGER PRIMARY KEY, \
            name TEXT DEFAULT \"Network\", \
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

		PdhAddEnglishCounter(queryHandle, L"\\Network Interface(*)\\Bytes Sent/sec", 0, &bytesSentCounter);
		PdhAddEnglishCounter(queryHandle, L"\\Network Interface(*)\\Bytes Received/sec", 0, &bytesReceivedCounter);
		PdhAddEnglishCounter(queryHandle, L"\\Network Interface(*)\\Bytes Total/sec", 0, &bytesTotalCounter);
		PdhAddEnglishCounter(queryHandle, L"\\Network Interface(*)\\Current Bandwidth", 0, &currentBandwidthCounter);
		PdhAddEnglishCounter(queryHandle, L"\\Network Interface(*)\\Packets Received/sec", 0, &packetsReceivedCounter);
		PdhAddEnglishCounter(queryHandle, L"\\Network Interface(*)\\Packets Sent/sec", 0, &packetsSentCounter);
		PdhAddEnglishCounter(queryHandle, L"\\Network Interface(*)\\Connections Active", 0, &connectionsActiveCounter);
		PdhAddEnglishCounter(queryHandle, L"\\Network Interface(*)\\Connections Established", 0, &connectionsEstablishedCounter);
		PdhAddEnglishCounter(queryHandle, L"\\Network Interface(*)\\Network Error/sec", 0, &networkErrorsCounter);

		PdhCollectQueryData(queryHandle);
	}

	virtual void RetrieveMetricValue(UINT16 counter) override {
		// Save the data to the database
		latestValue = std::make_shared<Metric>();
		latestValue->name = "Network";
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
};


