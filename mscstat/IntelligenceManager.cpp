#include "IntelligenceManager.h"
#include "Application.h"

void IntelligenceManager::Start() {
    active = true;

    while (active) {
        UINT interval = Application::theApp->configManager->GetConfig().predictionInterval;
        Predict();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
}