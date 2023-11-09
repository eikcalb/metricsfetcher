#include "ScriptManager.h"
#include "Application.h"

void ScriptManager::Process(UINT64 counter) {
    for (auto& script : scripts) {
        Application::theApp->threadManager->AddTaskToThread([&] {
            try {
                if (!should_stop.load()) {
                    script->metricCounter = counter;
                    // All scripts must define a single function called `execute`.
                    script->CallJavaScriptFunction("execute");
                }
            }
            catch (std::exception e) {
                // Ignore error and move to next script
                LogManager::GetInstance().LogError("Failed to call `execute` function in script: {0}", e.what());
            }
            });
    }

    const auto interval = intervalMS_ < 1000 ? 1000 : intervalMS_;
    LogManager::GetInstance().LogInfo("Scripts Run counter: {0}", counter + 1);
}
