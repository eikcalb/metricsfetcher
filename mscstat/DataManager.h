#pragma once
#include <sqlite3.h>
#include <vector>

#include "Utils.h"

class Row {
public:
	Row(sqlite3_stmt* stmt) : stmt_(stmt) {}

	// Get an integer value from the row
	int GetInt(int columnIndex) const {
		return sqlite3_column_int(stmt_, columnIndex);
	}

	// Get a double value from the row
	double GetDouble(int columnIndex) const {
		return sqlite3_column_double(stmt_, columnIndex);
	}

	// Get a string value from the row
	std::string GetString(int columnIndex) const {
		const char* columnValue = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, columnIndex));
		return columnValue ? columnValue : "";
	}

private:
	sqlite3_stmt* stmt_;
};

class DataManager {
public:


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
