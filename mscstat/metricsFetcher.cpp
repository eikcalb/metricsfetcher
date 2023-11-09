// mscstat.cpp : This file contains the 'main' function. Program execution begins and ends there.
#define _HAS_STD_BYTE 0
#include <iostream>
#include "Application.h"

using namespace rapidjson;

int main()
{
    auto &app = Application::CreateInstance();
    app.Initialize();

    app.Run();

    return 0;
}