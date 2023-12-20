// mscstat.cpp : This file contains the 'main' function. Program execution begins and ends there.
#define _HAS_STD_BYTE 0
#include <iostream>
#include "Application.h"

using namespace rapidjson;

int main()
{
    try {
        auto& app = Application::CreateInstance();
        app.Initialize();

        app.Run();
    }
    catch (std::exception ex) {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}