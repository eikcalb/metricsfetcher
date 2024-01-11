#pragma once
#pragma warning(disable : 4996) // Disable warning C4996
#include <atomic>
#include <memory>
#include <restbed>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <stdarg.h>
#include <filesystem>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <rapidjson/document.h>

#include "ConfigManager.h"
#include "LogManager.h"

using namespace rapidjson;
using namespace restbed;

class Server
{
private:
    void getIndexHandler(const std::shared_ptr< Session >& session);

    void getConfigHandler(const std::shared_ptr< Session >& session);
    void putConfigHandler(const std::shared_ptr< Session >& session);
    void getCounterHandler(const std::shared_ptr< Session >& session);

    void getScriptsHandler(const std::shared_ptr< Session >& session);
    void postScriptHandler(const std::shared_ptr< Session >& session);
    void patchScriptHandler(const std::shared_ptr< Session >& session);
    void deleteScriptHandler(const std::shared_ptr< Session >& session);

    void GetProvidersData(const std::shared_ptr< Session >& session);
    void GetProviderAggregateData(const std::shared_ptr< Session >& session);

    void getHealthHandler(const std::shared_ptr< Session >& session);

public:
    Server(USHORT _port) {
        port = _port;
        // Initialize the Restbed service
        service = std::make_shared<Service>();
        service->set_error_handler(errorHandler);
    }

    void Start() {
        auto settings = std::make_shared< Settings >();
        settings->set_port(port);
        settings->set_root("/");
        settings->set_bind_address("127.0.0.1");
        settings->set_default_header("Connection", "keep-alive");
        settings->set_default_header("User-Agent", "eikcalb server: 1.0");

        LogManager::GetInstance().LogInfo("Starting server on port: {0}", settings->get_port());

        //auto sslSettings = std::make_shared<SSLSettings>();
        //sslSettings->set_http_disabled(false);
        //sslSettings->set_tlsv12_enabled(true);
        //sslSettings->set_tlsv11_enabled(true);
        //sslSettings->set_private_key(Uri("file://./server.key"));
        //sslSettings->set_certificate(Uri("file://./server.crt"));
        //sslSettings->set_temporary_diffie_hellman(Uri("file://./dh768.pem"));
        //settings->set_ssl_settings(sslSettings);

#pragma region SetupRoutes
        // API routes
        service->publish(createRouteResource("/api/config", "GET", [&](const std::shared_ptr< Session >& session) { getConfigHandler(session); }));
        service->publish(createRouteResource("/api/config/save", "PUT", [&](const std::shared_ptr< Session >& session) { putConfigHandler(session); }));
        service->publish(createRouteResource("/api/counters", "GET", [&](const std::shared_ptr< Session >& session) { getCounterHandler(session); }));

        service->publish(createRouteResource("/api/script", "GET", [&](const std::shared_ptr< Session >& session) { getScriptsHandler(session); }));
        service->publish(createRouteResource("/api/script/save", "POST", [&](const std::shared_ptr< Session >& session) { postScriptHandler(session); }));
        service->publish(createRouteResource("/api/script/patch", "PATCH", [&](const std::shared_ptr< Session >& session) { patchScriptHandler(session); }));
        service->publish(createRouteResource("/api/script/delete/{name: .*}", "DELETE", [&](const std::shared_ptr< Session >& session) { deleteScriptHandler(session); }));

        service->publish(createRouteResource("/api/providers/{limit: \\d*}", "GET", [&](const std::shared_ptr< Session >& session) { GetProvidersData(session); }));
        service->publish(createRouteResource("/api/provider/aggregate", "GET", [&](const std::shared_ptr< Session >& session) { GetProviderAggregateData(session); }));

        service->publish(createRouteResource("/api/health", "GET", [&](const std::shared_ptr< Session >& session) { getHealthHandler(session); }));

        // Web routes
        service->publish(webAppResource);
#pragma endregion

        service->set_not_found_handler([&](const std::shared_ptr< Session >& session) { getIndexHandler(session); });
        service->start(settings);
    }

    void SetupWebPaths(std::set<std::string> paths) {
        auto resource = std::make_shared< Resource >();

        resource->set_paths(paths);
        resource->set_method_handler("GET", [&](const std::shared_ptr< Session >& session) { getIndexHandler(session); });
        resource->set_default_header("Access-Control-Allow-Origin", "*");
        resource->set_default_header("Access-Control-Allow-Headers", "*");
        //resource->set_default_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE");

        resource->set_method_handler("OPTIONS", [&](const std::shared_ptr< Session >& session) {
            if (session->get_request()->get_method() == "OPTIONS") {
                session->close(200, "OK", {
                    { "Access-Control-Allow-Methods", "GET, OPTIONS" },
                    });
            }
            }
        );

        LogManager::GetInstance().LogInfo("Setting up web routes for \"{0}\" paths found.", paths.size());
        webAppResource = resource;
    }

    // Stop the server
    void Stop() {
        service->stop();
    }

private:
    static std::string read_file(const std::string& filename) {
        std::filesystem::path filePath(filename);

        std::ifstream file(filePath);
        if (file) {
            std::ostringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
        return "";
    }

    static std::string get_content_type(const std::string& file_path) {
        size_t dot_position = file_path.find_last_of('.');
        if (dot_position != std::string::npos && dot_position < file_path.length() - 1) {
            std::string extension = file_path.substr(dot_position + 1);
            if (extension == "html") {
                return "text/html";
            }
            else if (extension == "css") {
                return "text/css";
            }
            else if (extension == "js") {
                return "application/javascript";
            }
            else if (extension == "ico") {
                return "image/x-icon";
            }
            else if (extension == "svg") {
                return "image/svg+xml";
            }
            else if (extension == "woff2") {
                return "font/woff2";
            }
        }
        return "text/html"; // Fallback to HTML if the extension is not recognized
    }

    static void errorHandler(const int, const std::exception& ex, const std::shared_ptr< Session > session)
    {
        LogManager::GetInstance().LogError("Server Error: {0}", ex.what());
        if (session && session->is_open()) {
            session->close(500, "Server Error", {
                { "Content-Type", "text/plain"},
                { "Content-Length", "36" } });
        }
    }

    static std::shared_ptr<Resource> createRouteResource(const std::string path, const std::string method, const std::function< void(const std::shared_ptr< Session >) >& callback)
    {
        auto resource = std::make_shared< Resource >();
        resource->set_path(path);
        resource->set_method_handler(method, callback);
        resource->set_default_header("Access-Control-Allow-Origin", "*");
        resource->set_default_header("Access-Control-Allow-Headers", "*");
        //resource->set_default_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE");

        resource->set_method_handler("OPTIONS", [&, method](const std::shared_ptr< Session >& session) {
            if (session->get_request()->get_method() == "OPTIONS") {
                session->close(200, "OK", {
                    { "Access-Control-Allow-Methods", method + ", OPTIONS" },
                    });
            }
            }
        );

        return resource;
    }

private:
    USHORT port;
    std::shared_ptr<Resource> webAppResource;

    std::shared_ptr<restbed::Service> service;
};

