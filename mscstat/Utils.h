#pragma once
#include <algorithm>
#include <zip.h>
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
#include <filesystem>
#include <set>
#include <fstream>
#include <sstream>


#include "LogManager.h"

class Utils
{
public:
    static bool ExtractZip(std::string source, std::string destination);
    
    static std::string GetAppDataPath();

    static bool IsPythonInstalled() {
        int pythonInstalled = system("python --version");
        return pythonInstalled == 0;
    }

    static void CheckAndInstallPythonDependencies(const std::string& requirementsFile) {
        std::string checkDependenciesCommand = "python -c \"import sys, pkgutil; exit_code = all(pkgutil.find_loader(package) is not None for package in sys.argv[1:]); sys.exit(0 if exit_code else 1)\" ";

        if (requirementsFile.c_str() != nullptr) {
            checkDependenciesCommand += "-r ";
            checkDependenciesCommand += requirementsFile;
        }

        int dependenciesInstalled = system(checkDependenciesCommand.c_str());

        // Install dependencies using pip if not installed
        if (dependenciesInstalled != 0) {
            LogManager::GetInstance().LogInfo("Installing dependencies Python...");
            std::string installDependenciesCommand = "python -m pip install -r \"";
            installDependenciesCommand += requirementsFile;
            installDependenciesCommand += "\"";

            int installResult = system(installDependenciesCommand.c_str());

            if (installResult != 0) {
                LogManager::GetInstance().LogError("Failed to install Python dependencies.");
                throw std::runtime_error("Failed to install python dependencies.");
            }
        }

        LogManager::GetInstance().LogInfo("Python dependencies installed successfully.");
    }

    static bool FileExists(const std::string& filePath) {
        return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath);
    }

    static bool FolderExists(const std::string& folderPath) {
        return std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath);
    }

    static std::set<std::string> GetAllFilePaths(const std::string& folderPath) {
        std::set<std::string> filePaths;

        try {
            // Iterate over all files and directories in the given path
            for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
                // Check if the entry is a regular file (not a directory)
                if (entry.is_regular_file()) {
                    // Get the path as a string and add it to the set
                    std::filesystem::path fullFilePath(entry.path().string());

                    // Extract the relative path
                    std::filesystem::path relativePath = fullFilePath.lexically_relative(folderPath);
                    auto pathString = relativePath.string();
                    std::replace(pathString.begin(), pathString.end(), '\\', '/');

                    filePaths.insert(pathString);

                    // Check if the file is html and add the path without it's extension
                    // This is because Restbed server will not match without explicitly
                    // configuring this.
                    if (relativePath.extension() == ".html") {
                        relativePath.replace_extension();
                        pathString = relativePath.string();
                        std::replace(pathString.begin(), pathString.end(), '\\', '/');

                        filePaths.insert(pathString);
                    }
                }
            }
        }
        catch (const std::exception& e) {
            LogManager::GetInstance().LogError("Error reading directories at path \"{0}\". Error: {1}", folderPath, e.what());
        }

        return filePaths;
    }

    static std::string GetExecutableDir() {
        char buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        return std::filesystem::path(buffer).parent_path().string();
    }

    static void NotifyUser(const std::string title, const std::string& message, const UINT type = MB_ICONINFORMATION) {
        UINT mask;

        if (MB_ICONINFORMATION == (type & MB_ICONINFORMATION)) {
            mask = MB_ICONINFORMATION;
        }
        else if (MB_ICONERROR == (type & MB_ICONERROR)) {
            mask = MB_ICONERROR;
        }
        else if (MB_ICONQUESTION == (type & MB_ICONQUESTION)) {
            mask = MB_ICONQUESTION;
        }
        else {
            mask = MB_ICONWARNING;
        }

        MessageBeep(mask);
        MessageBoxA(NULL, message.c_str(), title.c_str(), type | MB_OK | MB_TASKMODAL | MB_SERVICE_NOTIFICATION);
    }

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
        if (!output.empty() && output.size() > 3) {
            output.pop_back();
            output.pop_back();
            output.pop_back();
        }

        return output;
    }

    static std::string ExecutePythonFile(const std::string& filePath, const std::string& arguments = "") {
        std::string output;
        std::string command = "python \"";
        command += filePath;
        command += "\" ";
        command += arguments;

        LogManager::GetInstance().LogInfo("Executing Python file: {0}", command);

        FILE* pipe = _popen(command.c_str(), "r");

        if (!pipe) {
            throw std::runtime_error("Failed to open shell pipe.");
        }

        try {
            char buffer[1024];
            while (!feof(pipe)) {
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                    output += buffer;
            }
        } catch (std::exception e) {
            _pclose(pipe);
            LogManager::GetInstance().LogError("Failed to execute python file: {0}", filePath);
            throw e;
        }

        // Close the pipe
        const auto exitCode = _pclose(pipe);
        if (exitCode != 0) {
            LogManager::GetInstance().LogError("Failed to execute python file: {0}", filePath);
            throw std::runtime_error("Failed to execute process for python file: " + output);
        }

        return output;
    }

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

    static int* ReadIntegersFromFile(const std::string& filename) {
        int integers[] = {0,0,0,0,0};

        // Open the file
        std::ifstream file(filename);
        if (!file.is_open()) {
            LogManager::GetInstance().LogError("Error opening file: {0}", filename);
            return integers;
        }

        std::string file_content;
        std::getline(file, file_content);

        // Close the file
        file.close();

        // Use a stringstream to parse the comma-separated integers
        std::stringstream ss(file_content);

        for (int i = 0; i < 5; ++i) {
            if (!(ss >> integers[i])) {
                LogManager::GetInstance().LogWarning("Incorrect format found");
                return integers;
            }

            char comma;
            if (i < 5 - 1 && !(ss >> comma && comma == ',')) {
                LogManager::GetInstance().LogWarning("Incorrect format found");
                return integers;
            }
        }

        return integers;
    }
};

