#include "Server.h"

void Server::getIndexHandler(const shared_ptr< Session >& session)
{
    const auto request = session->get_request();

    ifstream stream("./index.html", ifstream::in);

    if (stream.is_open())
    {
        const string body = string(istreambuf_iterator< char >(stream), istreambuf_iterator< char >());

        const multimap< string, string > headers
        {
            { "Content-Type", "text/html" },
            { "Content-Length", ::to_string(body.length()) }
        };

        session->close(OK, body, headers);
    }
    else
    {
        session->close(NOT_FOUND);
    }
}

void Server::getGraphHandler(const shared_ptr< Session >& session)
{
    const auto request = session->get_request();
    const string filename = request->get_path_parameter("metricName");

    ifstream stream("./graph." + filename + ".html", ifstream::in);

    if (stream.is_open())
    {
        const string body = string(istreambuf_iterator< char >(stream), istreambuf_iterator< char >());

        const multimap< string, string > headers
        {
            { "Content-Type", "text/html" },
            { "Content-Length", ::to_string(body.length()) }
        };

        session->close(OK, body, headers);
    }
    else
    {
        session->close(NOT_FOUND);
    }
}


void Server::getMetricHandler(const shared_ptr< Session >& session)
{
    const auto request = session->get_request();
    const string filename = request->get_path_parameter("metricName");

    ifstream stream("./graph." + filename + ".html", ifstream::in);

    if (stream.is_open())
    {
        const string body = string(istreambuf_iterator< char >(stream), istreambuf_iterator< char >());

        const multimap< string, string > headers
        {
            { "Content-Type", "text/html" },
            { "Content-Length", ::to_string(body.length()) }
        };

        session->close(OK, body, headers);
    }
    else
    {
        session->close(NOT_FOUND);
    }
}
