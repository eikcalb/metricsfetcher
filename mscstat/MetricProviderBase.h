#pragma once
#include <chrono>
#include <pdh.h>
#include <sstream>
#include <pdhmsg.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "DataManager.h"
#include "Utils.h"

class MetricProviderBase {
public:
    virtual rapidjson::Value GetDataJSON(rapidjson::Document& doc, const UINT8 count) const = 0;
    virtual rapidjson::Value GetAggregateDataJSON(rapidjson::Document& doc, const std::string column) const = 0;
    virtual std::string GetName() = 0;
    // Returns `true` if this metric supports multiple values in its table.
    // `false` otherwise.
    virtual bool IsMulti() { return false; };

    virtual ~MetricProviderBase() {
        PdhCloseQuery(queryHandle);
    }

    // Retrieves a metric from the device. This will be implemented by subclasses
    // to retrieve and persist metric data. It accepts a counter that will be
    // used in combination with the timestamp to identify patterns across metrics.
    // if 2 metric values have the same counter value, it indicates that they were
    // read within the same loop. This should be used with the timestamp for finer
    // grouping beteween various metric values.
    // 
    // @param counter is a counter that indicates what point of the loop
    // this metric was fetched.
    virtual void RetrieveMetricValue(UINT16 counter) = 0;

protected:
    // Saves the metric value using the data storage defined by subclasses.
    virtual void Persist() {};

    bool CollectData() {
        PDH_STATUS status;

        status = PdhCollectQueryData(queryHandle);
        if (status != ERROR_SUCCESS) {
            return false;
        }

        return true;
    };

    double GetCounterValue(PDH_HCOUNTER counter) {
        PDH_FMT_COUNTERVALUE value;

        PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value);
        if (value.CStatus == PDH_CSTATUS_VALID_DATA) {
            return value.doubleValue;
        }
        else {
            return 0.0;
        }
    }

protected:
    PDH_HQUERY queryHandle = nullptr;
};
