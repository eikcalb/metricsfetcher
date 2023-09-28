#include "ScriptManager.h"
#include "Application.h"

void ScriptManager::Process() {
    {
        static UINT64 metricCounter = 0;

        while (!should_stop.load()) {
            for (auto& script : scripts) {
                Application::theApp->threadManager->AddTaskToThread([&] {
                    try {
                        script->metricCounter = metricCounter;
                        // All scripts must define a single function called `execute`.
                        script->CallJavaScriptFunction("execute");
                    }
                    catch (std::exception e) {
                        // Ignore error and move to next script
                        LogManager::GetInstance().LogError("Failed to call `execute` function in script");
                    }
                    });
            }

            metricCounter++;

            const auto interval = intervalMS_ < 1000 ? 1000 : intervalMS_;
            LogManager::GetInstance().LogInfo("Scripts Run: {0}. Next fetch in {1} seconds. ", metricCounter, interval / 1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    }
}