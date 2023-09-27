#pragma once
#include <chrono>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

class LogManager {
public:
    static LogManager& GetInstance() {
        static LogManager instance;
        return instance;
    }

    void LogDebug(const std::string& message) {
        logger->debug(message);
    }

    void LogInfo(const std::string& message) {
        logger->info(message);
    }

    void LogWarning(const std::string& message) {
        logger->warn(message);
    }

    void LogError(const std::string& message) {
        logger->error(message);
    }

    void LogCritical(const std::string& message) {
        logger->critical(message);
    }

    template <typename... Args>
    void LogDebug(const std::string& format, const Args&... args) {
        logger->debug(fmt::format(format, args...));
    }

    template <typename... Args>
    void LogInfo(const std::string& format, const Args&... args) {
        logger->info(fmt::format(format, args...));
    }

    template <typename... Args>
    void LogWarning(const std::string& format, const Args&... args) {
        logger->warn(fmt::format(format, args...));
    }

    template <typename... Args>
    void LogError(const std::string& format, const Args&... args) {
        logger->error(fmt::format(format, args...));
    }

    template <typename... Args>
    void LogCritical(const std::string& format, const Args&... args) {
        logger->critical(fmt::format(format, args...));
    }

private:
    LogManager();

    ~LogManager() {
        logger->flush();
    }

    std::shared_ptr<spdlog::logger> logger;
};

