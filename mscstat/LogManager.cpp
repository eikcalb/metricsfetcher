#include "LogManager.h"
#include "Application.h"
#include "Utils.h"

LogManager::LogManager() {
    constexpr size_t maxFileSize = static_cast<size_t>(1048576) * 1; // 1mb
    constexpr size_t maxFiles = 5;
    const auto& app = Application::theApp;

    auto consoleSink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(Utils::GetAppDataPath() + "\\logs.txt", maxFileSize, maxFiles);

    logger = std::make_shared<spdlog::logger>(app->name, spdlog::sinks_init_list{consoleSink, fileSink});

    if (!logger) {
        spdlog::critical("Could not create file logger. Cannot continue application");
        throw std::exception("Failed to initialize logger!");
    }

    spdlog::set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);

    spdlog::flush_every(std::chrono::seconds(10));
}