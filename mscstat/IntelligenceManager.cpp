#include "IntelligenceManager.h"
#include "Application.h"

void IntelligenceManager::Start() {
    active = true;

    LogManager::GetInstance().LogInfo("Starting intelligence manager.");
    auto errorCount = 0;

    while (active) {
        try {
            UINT interval = Application::theApp->configManager->GetConfig().predictionInterval;
            Predict();
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            errorCount = 0;
        }
        catch (std::exception e) {
            LogManager::GetInstance().LogError("Intelligence manager loop failed: {0}", e.what());
            if (errorCount > 3) {
                active = false;
                break;
            }
            errorCount++;
        }
    }
} 