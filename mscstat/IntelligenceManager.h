#pragma once
#include <string>

#include "Utils.h"

class IntelligenceManager {
public:
    static IntelligenceManager& GetInstance() {
        static IntelligenceManager instance(Utils::GetAppDataPath() + "\\www", Utils::GetAppDataPath() + "\\model");
        return instance;
    }

    IntelligenceManager(const IntelligenceManager&) = delete;
    IntelligenceManager& operator=(const IntelligenceManager&) = delete;

    std::string GetWebRoot() {
        return webRoot;
    }

    // This will start the prediction service and will run in at intervals to
    // compute and predict the device health using the module loaded.
    // This is a blocking call and should be done in a different thread.
    void Start(UINT interval) {
        
    }

private:
    std::string webRoot;
    std::string modelFullPath;

private:
    IntelligenceManager(const std::string webPath, const std::string modelPath) {
        webRoot = webPath;
        modelFullPath = modelPath;

        // Check if web root has been created, creat otherwise
        if (!Utils::FolderExists(webRoot)) {
            CreateWebRoot();
        }
        else {
            LogManager::GetInstance().LogInfo("Web root already created...Moving on.");
        }

        if (!Utils::FolderExists(modelFullPath)) {
            CreateModelRoot();
        }
        else {
            LogManager::GetInstance().LogInfo("Model root already created...Moving on.");
        }

        SetupPythonDependencies();
    }

    ~IntelligenceManager() {
    }

    void CreateWebRoot() {
        std::string sourcePath = Utils::GetExecutableDir() + "\\www.zip";

        LogManager::GetInstance().LogInfo("About to create web root");
        LogManager::GetInstance().LogInfo("\tFrom: {0}", sourcePath);
        LogManager::GetInstance().LogInfo("\tTo: {0}", webRoot);

        // Check if the operation was successful
        if (!Utils::ExtractZip(sourcePath, webRoot)) {
            LogManager::GetInstance().LogError("Web root creation failed: {0}", GetLastError());
            throw std::runtime_error("Failed to create web root.");
        }
        LogManager::GetInstance().LogInfo("Web root created successfully!");
    }

    void CreateModelRoot() {
        // Load the model
        std::string sourcePath = Utils::GetExecutableDir() + "\\model.zip";

        LogManager::GetInstance().LogInfo("About to create model root");
        LogManager::GetInstance().LogInfo("\tFrom: {0}", sourcePath);
        LogManager::GetInstance().LogInfo("\tTo: {0}", modelFullPath);

        // Check if the operation was successful
        if (!Utils::ExtractZip(sourcePath, modelFullPath)) {
            LogManager::GetInstance().LogError("Model root creation failed: {0}", GetLastError());
            throw std::runtime_error("Failed to create model root.");
        }
        LogManager::GetInstance().LogInfo("Model root created successfully!");
    }

    void SetupPythonDependencies() {
        if (!Utils::IsPythonInstalled()) {
            LogManager::GetInstance().LogInfo("Python not found.");
            throw std::runtime_error("Python is required to run this program.");
        }

        Utils::CheckAndInstallPythonDependencies(Utils::GetExecutableDir() + "\\requirements.txt");
        LogManager::GetInstance().LogInfo("Python dependencies setup successfully!");
    }

    void Predict() {
        // Code to run prediction is saved in python.
        // Here, we run the file.
        const auto reaponse = Utils::ExecutePythonFile(Utils::GetExecutableDir() + "\\predict.py");

        // Parse the prediction received from python.

    }
};

