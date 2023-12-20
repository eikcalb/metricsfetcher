#pragma once
#pragma warning(disable : 4996) // Disable warning C4996
#include <atomic>
#include <memory>
#include <restbed>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <stdarg.h>
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
    void getGraphHandler(const std::shared_ptr< Session >& session);
    void getMetricHandler(const std::shared_ptr< Session >& session);
    
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

    static void get_method_handler(const std::shared_ptr< Session > session)
    {
        const std::string resp = "Hello, World!";
        session->close(OK, resp, {
            { "Content-Type", "text/plain"},
            { "Content-Length", std::to_string(resp.length()) }
            });
    }

    void Start() {
        auto settings = std::make_shared< Settings >();
        settings->set_port(port);
        settings->set_bind_address("127.0.0.1");
        settings->set_default_header("Connection", "keep-alive");
        settings->set_default_header("User-Agent", "eikcalb server: 1.0");

        LogManager::GetInstance().LogInfo("Starting server on port: {0}", settings->get_port());

        auto sslSettings = std::make_shared<SSLSettings>();
        sslSettings->set_http_disabled(false);
        //sslSettings->set_tlsv12_enabled(true);
        //sslSettings->set_tlsv11_enabled(true);
        //sslSettings->set_private_key(Uri("file://./server.key"));
        //sslSettings->set_certificate(Uri("file://./server.crt"));
        //sslSettings->set_temporary_diffie_hellman(Uri("file://./dh768.pem"));
        settings->set_ssl_settings(sslSettings);

#pragma region SetupRoutes
        // Web routes
        service->publish(createRouteResource("/", "GET", [&](const std::shared_ptr< Session >& session) {
            getIndexHandler(session);
            }));
        service->publish(createRouteResource("/graph/{metricName: [a-z]*}", "GET", [&](const std::shared_ptr< Session >& session) { getGraphHandler(session); }));
        service->publish(createRouteResource("/metric/{metricName: [a-z]*}", "GET", [&](const std::shared_ptr< Session >& session) { getMetricHandler(session); }));

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

#pragma endregion

        service->start(settings);
    }

    // Stop the server
    void Stop() {
        service->stop();
    }

private:
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

    std::shared_ptr<restbed::Service> service;
};

