#pragma once
#include <iostream>
#include <string>
#include <Windows.h>
#include <Knownfolders.h>
#include <ShlObj.h>
#include <vector>
#include <iphlpapi.h>
#include <functional>
#include "LogManager.h"
#include <codecvt>

class Utils
{
public:
    static std::string GetAppDataPath();

    static std::string GetActiveProcessTitle() {
        HWND hwnd = GetForegroundWindow(); // Get the handle of the focused window

        if (hwnd != NULL) {
            DWORD processId;
            GetWindowThreadProcessId(hwnd, &processId); // Get the process ID

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId); // Open the process

            if (hProcess != NULL) {
                WCHAR processName[MAX_PATH];
                DWORD processNameLength = sizeof(processName) / sizeof(processName[0]);

                if (QueryFullProcessImageName(hProcess, 0, processName, &processNameLength) != 0) {
                    // Convert the process name to a std::wstring
                    std::wstring wProcessName(processName);

                    // Extract the actual executable filename
                    size_t lastSlashPos = wProcessName.find_last_of(L"\\");
                    if (lastSlashPos != std::wstring::npos) {
                        wProcessName = wProcessName.substr(lastSlashPos + 1);
                    }

                    // Convert the std::wstring to a std::string
                    std::string processNameStr(wProcessName.begin(), wProcessName.end());

                    return processNameStr;
                }
                else {
                    LogManager::GetInstance().LogError("QueryFullProcessImageName failed.");
                }

                CloseHandle(hProcess); // Close the process handle
            }
            else {
                LogManager::GetInstance().LogError("OpenProcess failed.");
            }
        }
        else {
            LogManager::GetInstance().LogError("GetForegroundWindow failed.");
        }

        return "";
    }

    static std::string GetActiveWindowTitle() {
        HWND hwnd = GetForegroundWindow();
        if (hwnd == NULL) {
            return "";
        }

        const int bufferSize = 1024;
        wchar_t buffer[bufferSize];

        int length = GetWindowText(hwnd, buffer, bufferSize);
        if (length == 0) {
            return "";
        }

        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(buffer);
    }

    static void EnumNetworkInterfaces(std::function<void(std::string)> callback) {
        // Define variables for storing network interface information
        ULONG ulOutBufLen = 0;
        DWORD dwRetVal = 0;

        // Call GetAdaptersInfo to retrieve network interface information
        PIP_ADAPTER_INFO pAdapterInfo = NULL;

        dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
            if (pAdapterInfo == NULL) {
                LogManager::GetInstance().LogError("Memory allocation failed.");
                return;
            }
        }

        dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
        if (dwRetVal == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter) {
                callback(pAdapter->Description);
                pAdapter = pAdapter->Next;
            }
        }
        else {
            LogManager::GetInstance().LogError("GetAdaptersInfo failed with error code: {0}", dwRetVal);
        }

        // Free allocated memory
        if (pAdapterInfo) {
            free(pAdapterInfo);
        }
    }
};

