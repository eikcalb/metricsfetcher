#include "Application.h"
#include "CPUMetricProvider.h"
#include "StorageMetricProvider.h"
#include "RAMMetricProvider.h"
#include "NetworkMetricProvider.h"
#include "ProcessMetricProvider.h"

volatile std::sig_atomic_t Application::g_signal_flag = 0;
std::shared_ptr<Application> Application::theApp = nullptr;

void Application::Initialize() {
    SetConsoleTitle(std::wstring(name.begin(), name.end()).c_str());
    // This is the main application dependency as all other object managers
    // require the configuration object.
    configManager = &ConfigManager::GetInstance();

    // Other parts of the code depend on the logger,
    // hence it is important to initialize it early.
    logManager = &LogManager::GetInstance();

    // This should be the final dependency required to startup the application
    // all other classes can be created in any order.
    dataManager = &DataManager::GetInstance();

    if (!dataManager->IsOpen()) {
        logManager->LogCritical("Database failed to open!");
        return;
    }

    CreateApplicationTable();
    configManager->LoadConfig(FetchConfigData());

    metricsManager = &MetricsManager::GetInstance(configManager->GetConfig().metricFetchInterval);
    threadManager = &ThreadManager::GetInstance(configManager->GetConfig().poolSize);
    scriptManager = &ScriptManager::GetInstance(configManager->GetConfig().metricFetchInterval);
    scriptManager->Initialize();

    aiManager = &IntelligenceManager::GetInstance();

    server = new Server(configManager->GetConfig().port);
    server->SetupWebPaths(Utils::GetAllFilePaths(aiManager->GetWebRoot()));
}

void Application::Run() {
    // Implementation of the application logic
    std::signal(SIGINT, SignalHandler); // Handle Ctrl+C (SIGINT)
    std::signal(SIGTERM, SignalHandler); // Handle termination request (SIGTERM)
    logManager->LogInfo("========================");
    logManager->LogInfo("= Application Started! =");
    logManager->LogInfo("========================");

    // Add metrics providers to the manager. This will control the orderly
    // generation of metrics and save each metric to the database.
    metricsManager->AddMetricProvider(std::make_unique<CPUMetricProvider>());
    metricsManager->AddMetricProvider(std::make_unique<ProcessMetricProvider>());
    metricsManager->AddMetricProvider(std::make_unique<RAMMetricProvider>());
    metricsManager->AddMetricProvider(std::make_unique<StorageMetricProvider>());

    // Get the network interfaces available on the device.
    Utils::EnumNetworkInterfaces([&](std::string interfaceName) {
        metricsManager->AddMetricProvider(std::make_unique<NetworkMetricProvider>(interfaceName));
        });

    while (!g_signal_flag) {
        try {
            threadManager->AddTaskToThread([this] {
                // Start collecting metrics on a dedicated thread.
                // For each metric, we want to collect values using a specific counter value,
                // therefore giving us insight to when the query was called.
                metricsManager->StartMetricsCollection();
                },
                ThreadType::METRICS_MANAGER_THREAD
            );
            threadManager->AddTaskToThread([this] {
                aiManager->Start();
                },
                ThreadType::METRICS_MANAGER_THREAD
            );

            logManager->LogInfo("Starting HTTP server.");
            // This is a blocking call that keeps the application server running
            server->Start();
        }
        catch (const std::exception& e) {
            Stop();
            logManager->LogCritical("Application crashed: {0}", e.what());
            break;
        }
    }

    Stop();
}

void Application::Stop() {
    logManager->LogInfo("= Stopping application =");

    metricsManager->StopMetricsCollection();
    scriptManager->Stop();
    server->Stop();
    threadManager->Stop();

    logManager->LogInfo("= Application stopped! =");
    logManager->LogInfo("========================");
}
