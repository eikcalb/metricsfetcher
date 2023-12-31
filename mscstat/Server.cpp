#include "Server.h"
#include "Application.h"

void Server::getIndexHandler(const std::shared_ptr< Session >& session)
{
    const auto request = session->get_request();

    std::ifstream stream("./index.html", std::ifstream::in);

    if (stream.is_open())
    {
        const std::string body = std::string(std::istreambuf_iterator< char >(stream), std::istreambuf_iterator< char >());

        const std::multimap< std::string, std::string > headers
        {
            { "Content-Type", "text/html" },
            { "Content-Length", std::to_string(body.length()) }
        };

        session->close(OK, body, headers);
    }
    else
    {
        session->close(NOT_FOUND);
    }
}

void Server::getGraphHandler(const std::shared_ptr< Session >& session)
{
    const auto request = session->get_request();
    const std::string filename = request->get_path_parameter("metricName");

    std::ifstream stream("./graph." + filename + ".html", std::ifstream::in);

    if (stream.is_open())
    {
        const std::string body = std::string(std::istreambuf_iterator< char >(stream), std::istreambuf_iterator< char >());

        const std::multimap< std::string, std::string > headers
        {
            { "Content-Type", "text/html" },
            { "Content-Length", std::to_string(body.length()) }
        };

        session->close(OK, body, headers);
    }
    else
    {
        session->close(NOT_FOUND);
    }
}


void Server::getMetricHandler(const std::shared_ptr< Session >& session)
{
    const auto request = session->get_request();
    const std::string filename = request->get_path_parameter("metricName");

    std::ifstream stream("./graph." + filename + ".html", std::ifstream::in);

    if (stream.is_open())
    {
        const std::string body = std::string(std::istreambuf_iterator< char >(stream), std::istreambuf_iterator< char >());

        const std::multimap< std::string, std::string > headers
        {
            { "Content-Type", "text/html" },
            { "Content-Length", std::to_string(body.length()) }
        };

        session->close(OK, body, headers);
    }
    else
    {
        session->close(NOT_FOUND);
    }
}

void Server::getConfigHandler(const std::shared_ptr< Session >& session)
{
    const auto config = ConfigManager::GetInstance().GetConfigAsJSON();

    session->close(OK, config, {
        { "Content-Type", "application/json"},
        { "Content-Length", std::to_string(config.length()) }
        });
}

void Server::putConfigHandler(const std::shared_ptr< Session >& session)
{
    try {
        std::string json_data;
        const auto req = session->get_request();
        size_t content_length = req->get_header("Content-Length", 0);

        session->fetch(content_length, [&json_data, req](const std::shared_ptr< Session > session, const Bytes& body)
            {
                json_data = String::to_string(body);
            });

        if (json_data.empty()) {
            throw std::runtime_error("You must provide a valid configuration");
        }

        LogManager::GetInstance().LogDebug("putConfigHandler: Received new config: {0}", json_data);

        if (!Application::theApp->SaveConfigData(json_data)) {
            throw std::runtime_error("Failed to save configuration");
        }

        const std::string responseData = "{\"success\": true}";
        session->close(OK, responseData, {
            { "Content-Type", "application/json"},
            { "Content-Length", std::to_string(responseData.length()) }
            });
    }
    catch (std::runtime_error e) {
        session->close(BAD_REQUEST, e.what(), {
            { "Content-Type", "text/plain"},
            { "Content-Length", std::to_string(strlen(e.what())) }
            });
    }
}

void Server::getCounterHandler(const std::shared_ptr< Session >& session)
{
    const auto response = Application::theApp->metricsManager->GetAvailableCountersJSON();

    session->close(OK, response, {
        { "Content-Type", "application/json"},
        { "Content-Length", std::to_string(response.length()) }
        });
}

void Server::getScriptsHandler(const std::shared_ptr< Session >& session)
{
    const auto scripts = Application::theApp->scriptManager->GetAllScriptsAsJSON();

    session->close(OK, scripts, {
        { "Content-Type", "application/json"},
        { "Content-Length", std::to_string(scripts.length()) }
        });
}

void Server::postScriptHandler(const std::shared_ptr< Session >& session)
{
    try {
        const auto req = session->get_request();
        size_t content_length = req->get_header("Content-Length", 0);

        session->fetch(content_length, [req](const std::shared_ptr< Session > session, const Bytes& body)
            {
                std::string json_data;
                json_data = String::to_string(body);
                if (json_data.empty()) {
                    throw std::runtime_error("You must provide script information");
                }
                LogManager::GetInstance().LogDebug("postScriptHandler: Received new script to save: {0}", json_data);
                if (!Application::theApp->scriptManager->SaveScript(json_data)) {
                    throw std::runtime_error("Failed to save script");
                }
                const std::string responseData = "{\"success\": true}";
                session->close(OK, responseData, {
                    { "Content-Type", "application/json"},
                    { "Content-Length", std::to_string(responseData.length()) }
                    });
            });
    }
    catch (std::runtime_error e) {
        session->close(BAD_REQUEST, e.what(), {
            { "Content-Type", "text/plain"},
            { "Content-Length", std::to_string(strlen(e.what())) }
            });
    }
}

void Server::patchScriptHandler(const std::shared_ptr< Session >& session)
{
    try {
        const auto req = session->get_request();
        size_t content_length = req->get_header("Content-Length", 0);

        session->fetch(content_length, [req](const std::shared_ptr< Session > session, const Bytes& body)
            {
                std::string json_data = String::to_string(body);
                if (json_data.empty()) {
                    throw std::runtime_error("You must provide script information");
                }
                LogManager::GetInstance().LogDebug("postScriptHandler: Received updated script to save: {0}", json_data);
                if (!Application::theApp->scriptManager->UpdateScript(json_data)) {
                    throw std::runtime_error("Failed to update script");
                }
                const std::string responseData = "{\"success\": true}";
                session->close(OK, responseData, {
                    { "Content-Type", "application/json"},
                    { "Content-Length", std::to_string(responseData.length()) }
                    });
            });
    }
    catch (std::runtime_error e) {
        session->close(BAD_REQUEST, e.what(), {
            { "Content-Type", "text/plain"},
            { "Content-Length", std::to_string(strlen(e.what())) }
            });
    }
}

void Server::deleteScriptHandler(const std::shared_ptr< Session >& session)
{
    try {
        const auto req = session->get_request();
        auto name = req->get_path_parameter("name");

        if (name.empty()) {
            throw std::runtime_error("You must provide a name");
        }

        LogManager::GetInstance().LogDebug("postScriptHandler: Received script name to delete: {0}", name);

        if (!Application::theApp->scriptManager->DeleteScript(name)) {
            throw std::runtime_error("Failed to delete script");
        }

        const std::string responseData = "{\"success\": true}";
        session->close(OK, responseData, {
            { "Content-Type", "application/json"},
            { "Content-Length", std::to_string(responseData.length()) }
            });
    }
    catch (std::runtime_error e) {
        session->close(BAD_REQUEST, e.what(), {
            { "Content-Type", "text/plain"},
            { "Content-Length", std::to_string(strlen(e.what())) }
            });
    }
}

void Server::GetProvidersData(const std::shared_ptr< Session >& session)
{
    try {
        const auto req = session->get_request();
        // Get the specified limit or use 50
        auto fetchLimit = std::stoi(req->get_path_parameter("limit", "50"));
        const auto scripts = Application::theApp->metricsManager->GetProviderDataJSON(fetchLimit);

        session->close(OK, scripts, {
            { "Content-Type", "application/json"},
            { "Content-Length", std::to_string(scripts.length()) }
            });
    } catch (std::runtime_error e) {
        session->close(BAD_REQUEST, e.what(), {
            { "Content-Type", "text/plain"},
            { "Content-Length", std::to_string(strlen(e.what())) }
            });
    }
}


void Server::GetProviderAggregateData(const std::shared_ptr< Session >& session)
{
    try {
        const auto& req = session->get_request();
        const auto & column = req->get_query_parameter("column");
        if (column.empty()) {
            throw std::runtime_error("Provide column in order to fetch aggregate.");
        }

        const bool isCustom = std::stoi(req->get_query_parameter("isCustom", "0")) == 1;
        const auto& name = req->get_query_parameter("name");
        const auto aggregate = Application::theApp->metricsManager->GetProviderAggregateDataJSON(column, isCustom, name);

        session->close(OK, aggregate, {
            { "Content-Type", "application/json"},
            { "Content-Length", std::to_string(aggregate.length()) }
            });
    }
    catch (std::runtime_error e) {
        session->close(BAD_REQUEST, e.what(), {
            { "Content-Type", "text/plain"},
            { "Content-Length", std::to_string(strlen(e.what())) }
            });
    }
}

void Server::getHealthHandler(const std::shared_ptr< Session >& session)
{
    try {
        const auto info = Application::theApp->metricsManager->GetInfoAsJSON();

        session->close(OK, info, {
            { "Content-Type", "application/json"},
            { "Content-Length", std::to_string(info.length()) }
            });
    } catch (std::runtime_error e) {
        session->close(BAD_REQUEST, e.what(), {
            { "Content-Type", "text/plain"},
            { "Content-Length", std::to_string(strlen(e.what())) }
            });
    }
}
