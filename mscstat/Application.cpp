#include "Application.h"
#include "CPUMetricProvider.h"
#include "StorageMetricProvider.h"
#include "RAMMetricProvider.h"
#include "NetworkMetricProvider.h"

volatile std::sig_atomic_t Application::g_signal_flag = 0;
std::shared_ptr<Application> Application::theApp = nullptr;

void Application::Initialize() {
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
}

void Application::Run() {
	// Implementation of the application logic
	std::signal(SIGINT, SignalHandler); // Handle Ctrl+C (SIGINT)
	std::signal(SIGTERM, SignalHandler); // Handle termination request (SIGTERM)

	logManager->LogInfo("====================");
	logManager->LogInfo("Application Started!");
	logManager->LogInfo("====================");

	// Add metrics providers to the manager. This will control the orderly
	// generation of metrics and save each metric to the database.
	metricsManager->AddMetricProvider(std::make_unique<CPUMetricProvider>());
	metricsManager->AddMetricProvider(std::make_unique<NetworkMetricProvider>());
	metricsManager->AddMetricProvider(std::make_unique<RAMMetricProvider>());
	metricsManager->AddMetricProvider(std::make_unique<StorageMetricProvider>());

	while (!g_signal_flag) {
		try {
			// Start collecting metrics on a dedicated thread.
			threadManager->AddTaskToThread([this] {
				metricsManager->StartMetricsCollection();
				},
				ThreadType::METRICS_MANAGER_THREAD
			);
			// TODO: Make blocking call to initialize the server.
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch (const std::exception& e) {
			// TODO: Kill all services
			metricsManager->StopMetricsCollection();

			logManager->LogCritical("Application crashed: {0}", e.what());
			break;
		}
	}

	// TODO: Kill all services
	metricsManager->StopMetricsCollection();
	logManager->LogInfo("Application Ended!");
}