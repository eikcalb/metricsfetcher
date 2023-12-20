#pragma once
#include <iostream>
#include <string>
#include <Windows.h>
#include <Knownfolders.h>
#include <ShlObj.h>
#include <vector>
#include <iphlpapi.h>
#include <functional>
#include <codecvt>
#include <rapidjson/document.h>

#include "LogManager.h"

class Utils
{
public:
    static rapidjson::Value ConvertDoubleToJSONValue(const double& value, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value rapidValue;
        rapidValue.SetDouble(value);

        return rapidValue;
    }

    static rapidjson::Value ConvertIntToJSONValue(const int& value, rapidjson::Document::AllocatorType& allocator) {
        rapidjson::Value rapidValue;
        rapidValue.SetInt(value);

        return rapidValue;
    }

    static std::vector<std::string> ExecuteShellCommand(const std::string& command) {
        std::vector<std::string> output;

        // Open a pipe to the shell process
        //FILE* pipe = _popen(command.c_str(), "r");
        FILE* pipe = _popen(("powershell.exe " + command).c_str(), "r");

        if (!pipe) {
            throw std::runtime_error("Failed to open shell pipe.");
        }

        try {
            char buffer[1024];

            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                output.emplace_back(buffer);
            }
        }
        catch (std::exception e) {
            _pclose(pipe);
            LogManager::GetInstance().LogError("Failed to run command.");
            throw e;
        }

        // Close the pipe
        _pclose(pipe);

        // Remove shell comment
        if (!output.empty()) {
            output.pop_back();
            output.pop_back();
            output.pop_back();
        }

        return output;
    }

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
        return WideStringToString(buffer, length);
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

    static std::string WideStringToString(const wchar_t* wideStr) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);

        return WideStringToString(wideStr, len);
    }

    static std::string WideStringToString(const wchar_t* wideStr, const int& len) {
        if (len > 0) {
            std::string result(len, 0);
            WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &result[0], len, nullptr, nullptr);
            return result;
        }

        return "";
    }

    static std::wstring StringToWstring(const std::string& str) {
        int wstrLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
        std::wstring wstr(wstrLen, 0);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], wstrLen);
        return wstr;
    }
};

