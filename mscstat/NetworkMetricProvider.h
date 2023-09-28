#pragma once
#include "MetricProviderBase.h"
#include <codecvt>

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
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::wstring wideString = converter.from_bytes(name);

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
