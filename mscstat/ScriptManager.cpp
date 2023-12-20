#include "ScriptManager.h"
#include "Application.h"

void ScriptManager::Process(std::atomic<UINT64>& counter) {
    // Because we create the JavaScript context each time this process is called,
    // we endup creating a non-threadsafe condition if this function is called
    // multiple times. To fix this, we will need to wait and ensure that this function
    // only returns after all scripts have been triggered.
    std::lock_guard<std::mutex> lock(scriptMutex);
    for (std::shared_ptr<Script>& script : scripts) {
        Application::theApp->threadManager->AddTaskToThread([&] {
            std::lock_guard<std::mutex> scriptLock(script->ctxMutex);
            try {
                if (!should_stop.load()) {
                    SetupJavascriptContext(script);
                    script->metricCounter = counter;
                    // All scripts must define a single function called `execute`.
                    script->CallJavaScriptFunction("execute");
                    script->ClearDuktapeStack();
                }
            }
            catch (std::exception e) {
                // Ignore error and move to next script
                LogManager::GetInstance().LogError("Failed to call `execute` function in script: {0}", e.what());
            }
            });
    }

    LogManager::GetInstance().LogInfo("Scripts Run counter: {0}. Script count: {1}", counter + 1, scripts.size());
}
