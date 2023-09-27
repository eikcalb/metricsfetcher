#pragma once
#include <iostream>
#include <string>
#include <Windows.h>
#include <Knownfolders.h>
#include <ShlObj.h>

class Utils
{
public:
    static std::string GetAppDataPath();

    static std::wstring GetActiveWindowTitle() {
        HWND hwnd = GetForegroundWindow();
        if (hwnd == NULL) {
            return L"";
        }

        const int bufferSize = 1024;
        wchar_t buffer[bufferSize];

        int length = GetWindowText(hwnd, buffer, bufferSize);
        if (length == 0) {
            return L"";
        }

        return std::wstring(buffer);
    }
};

