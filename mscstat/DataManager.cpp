#include "DataManager.h"
#include "Application.h"

DataManager::DataManager(const std::string& dbFileName) : dbFileName_(dbFileName), db_(nullptr) {
    int result = sqlite3_open(dbFileName.c_str(), &db_);
    if (result != SQLITE_OK) {
        Application::theApp->logManager->LogError("Failed to open database: {0}", sqlite3_errmsg(db_));
        db_ = nullptr; // Set to nullptr to indicate failure
    }
}

bool DataManager::ExecuteSQLStatement(const std::string& sql) {
    if (!IsOpen()) {
        return false;
    }

    char* errMsg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        Application::theApp->logManager->LogError("SQL error: {0}. SQL: {1}", errMsg, sql);
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}


std::vector<Row> DataManager::ExecuteSelect(const std::string& sql) {
    std::vector<Row> resultData;

    if (!IsOpen()) {
        return resultData;
    }

    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    if (result != SQLITE_OK) {
        Application::theApp->logManager->LogError("SQL error: {0}. SQL: {1}", sqlite3_errmsg(db_), sql);
        return resultData;
    }

    // Execute the SELECT statement
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        auto columnValue2 = sqlite3_column_text(stmt, 2);

        resultData.emplace_back(stmt);
    }

    return resultData;
}