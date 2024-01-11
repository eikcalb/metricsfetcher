#pragma once
#include <string>

#include "Utils.h"

class IntelligenceManager {
public:
    static IntelligenceManager& GetInstance() {
        static IntelligenceManager instance(Utils::GetAppDataPath() + "\\www");
        return instance;
    }

    IntelligenceManager(const IntelligenceManager&) = delete;
    IntelligenceManager& operator=(const IntelligenceManager&) = delete;

    std::string GetWebRoot() {
        return webRoot;
    }

private:
    std::string webRoot;
private:
    IntelligenceManager(const std::string path) {
        webRoot = path;

        // Check if web root has been created, creat otherwise
        if (!Utils::FolderExists(webRoot)) {
            CreateWebRoot();
        }
        else {
            LogManager::GetInstance().LogInfo("Web root already created...Moving on.");
        }
    }

    void CreateWebRoot() {
        std::string sourcePath = Utils::GetExecutableDir() + "\\www.zip";

        LogManager::GetInstance().LogInfo("About to create web root");
        LogManager::GetInstance().LogInfo("\tFrom: {0}", sourcePath);
        LogManager::GetInstance().LogInfo("\tTo: {0}", webRoot);

        // Check if the operation was successful
        if (!Utils::ExtractZip(sourcePath, webRoot)) {
            throw std::runtime_error("Failed to create web root.");
            LogManager::GetInstance().LogError("Web root creation failed: {0}", GetLastError());
        }
        LogManager::GetInstance().LogInfo("Web root created successfully!");
    }
};

