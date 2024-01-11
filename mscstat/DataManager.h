#pragma once
#include <map>
#include <sqlite3.h>
#include <variant>
#include <vector>

#include "Utils.h"

class Row {
private:
    std::map <std::string, std::variant<int, double, std::string, std::vector<unsigned char>>> data_;

public:
    Row(sqlite3_stmt* stmt) {
        int numColumns = sqlite3_column_count(stmt);
        for (int i = 0; i < numColumns; i++) {
            const char* columnName = sqlite3_column_name(stmt, i);
            int columnType = sqlite3_column_type(stmt, i);

            switch (columnType) {
            case SQLITE_INTEGER:
                data_[columnName] = sqlite3_column_int(stmt, i);
                break;

            case SQLITE_FLOAT:
                data_[columnName] = sqlite3_column_double(stmt, i);
                break;

            default:
                const char* columnValue = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                if (columnValue) {
                    data_[columnName] = std::string(columnValue);
                }
                break;
            }
        }
    }

    // Retrieve the value by column name
    template <typename T>
    T GetValue(const std::string& columnName) const {
        auto it = data_.find(columnName);
        if (it != data_.end()) {
            return std::get<T>(it->second);
        }
        return T{};
    }

    // Get an integer value from the row
    int GetInt(const std::string& columnName) const {
        return GetValue<int>(columnName);
    }

    // Get a double value from the row
    double GetDouble(const std::string& columnName) const {
        return GetValue<double>(columnName);
    }

    // Get a string value from the row
    std::string GetString(const std::string& columnName) const {
        return GetValue<std::string>(columnName);
    }
};

class DataManager {
public:
    static DataManager& GetInstance() {
        static DataManager instance(Utils::GetAppDataPath() + "\\storage.db");
        return instance;
    }

    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    bool IsOpen() const {
        return db_ != nullptr;
    }

    bool CreateTable(const std::string& tableName, const std::string& columns) {
        std::string createTableSQL = "CREATE TABLE IF NOT EXISTS " + tableName + " (" + columns + ");";
        return ExecuteSQLStatement(createTableSQL);
    }

    bool Insert(const std::string& tableName, const std::vector<std::string>& values) {
        std::string insertSQL = "INSERT INTO " + tableName + " VALUES (" + JoinValues(values) + ");";
        return ExecuteSQLStatement(insertSQL);
    }

    bool Insert(const std::string& tableName, std::string& values) {
        std::string insertSQL = "INSERT INTO " + tableName + " VALUES (" + values + ");";
        return ExecuteSQLStatement(insertSQL);
    }

    bool Delete(const std::string& tableName, const std::string& condition = "") {
        std::string deleteSQL = "DELETE FROM " + tableName;
        if (!condition.empty()) {
            deleteSQL += " WHERE " + condition;
        }
        return ExecuteSQLStatement(deleteSQL);
    }

    bool Update(const std::string& tableName, const std::string& setClause, const std::string& condition = "") {
        std::string updateSQL = "UPDATE " + tableName + " SET " + setClause;
        if (!condition.empty()) {
            updateSQL += " WHERE " + condition;
        }
        return ExecuteSQLStatement(updateSQL);
    }

    bool Upsert(const std::string& tableName, const std::string& columns, const std::string& setClause) {
        std::string upsertSQL = "INSERT OR REPLACE INTO " + tableName + " (" + columns + ")" + " VALUES (" + setClause + ")";
        return ExecuteSQLStatement(upsertSQL);
    }

    std::vector<Row> SelectAggregate(const std::string& tableName, const std::string& column, const std::string& condition = "") {
        std::string selectSQL = "\
            SELECT \
            MAX(CAST(" + column + " AS REAL)) AS max, \
            MIN(CAST(" + column + " AS REAL)) AS min, \
            AVG(CAST(" + column + " AS REAL)) AS avg, \
            TOTAL(CAST(" + column + " AS REAL)) AS total, \
            COUNT(" + column + ") AS count FROM " + tableName;
        if (!condition.empty()) {
            selectSQL += " WHERE " + condition;
        }
        return ExecuteSelect(selectSQL);
    }

    std::vector<Row> Select(const std::string& tableName, const std::string& condition = "") {
        std::string selectSQL = "SELECT * FROM " + tableName;
        if (!condition.empty()) {
            selectSQL += " WHERE " + condition;
        }
        return ExecuteSelect(selectSQL);
    }
private:
    DataManager(const std::string& dbFileName);

    ~DataManager() {
        if (db_) {
            // Close the database when the DataManager is destroyed
            sqlite3_close(db_);
        }
    }

    bool ExecuteSQLStatement(const std::string& sql);

    std::vector<Row> ExecuteSelect(const std::string& sql);

    std::string JoinValues(const std::vector<std::string>& values, const std::string& separator = ", ") {
        std::string result;
        for (size_t i = 0; i < values.size(); ++i) {
            result += "'" + values[i] + "'";
            if (i < values.size() - 1) {
                result += separator;
            }
        }
        return result;
    }

private:
    std::string dbFileName_;
    sqlite3* db_;
};
