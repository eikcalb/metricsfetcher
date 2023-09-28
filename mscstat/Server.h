#pragma once
#pragma warning(disable : 4996) // Disable warning C4996
#include <atomic>
#include <memory>
#include <restbed>
#include <cstdlib>
#include <fstream>
#include <streambuf>
#include <iostream>

using namespace std;
using namespace restbed;

class Server
{

private:
	void getIndexHandler(const shared_ptr< Session >& session);
	void getGraphHandler(const shared_ptr< Session >& session);
	void getMetricHandler(const shared_ptr< Session >& session);

public:
	Server() {
		// Initialize the Restbed service
		service = make_shared<Service>();
	}

	void Start() {
		auto resource = make_shared< Resource >();
		resource->set_path("/");
		resource->set_method_handler("GET", [&](const shared_ptr< Session >& session) { getIndexHandler(session); });

		auto resource2 = make_shared< Resource >();
		resource2->set_path("/graph/{metricName: [a-z]*}");
		resource2->set_method_handler("GET", [&](const shared_ptr< Session >& session) { getGraphHandler(session); });

		auto resource3 = make_shared< Resource >();
		resource3->set_path("/metric/{metricName: [a-z]*}");
		resource3->set_method_handler("GET", [&](const shared_ptr< Session >& session) { getMetricHandler(session); });

		auto settings = make_shared< Settings >();
		settings->set_port(1284);
		settings->set_default_header("Connection", "close");

		auto sslSettings = make_shared< SSLSettings >();
		sslSettings->set_http_disabled(false);
		//sslSettings->set_passphrase("test");
		//sslSettings->set_private_key(Uri("file://./key.pem"));
		//sslSettings->set_certificate(Uri("file://./cert.pem"));
		//settings->set_ssl_settings(sslSettings);

		service->publish(resource);
		service->start(settings);
	}

	// Stop the server
	void Stop() {
		service->stop();
	}

private:
	std::shared_ptr<restbed::Service> service;
};

