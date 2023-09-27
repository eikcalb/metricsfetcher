// mscstat.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "spdlog/spdlog.h"
#include <spdlog/stopwatch.h>
#include "Application.h"

using namespace rapidjson;

int main()
{
    auto &app = Application::CreateInstance();
    app.Initialize();

    app.Run();

    return 0;
}