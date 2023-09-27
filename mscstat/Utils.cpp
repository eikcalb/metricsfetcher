#include "Utils.h"
#include "Application.h"

std::string Utils::GetAppDataPath() {
    const auto& appName = Application::theApp->name;

    std::wstring wideSubfolder(appName.begin(), appName.end());

    wchar_t* appDataPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appDataPath))) {
        std::wstring appDataFolderPath(appDataPath);
        CoTaskMemFree(appDataPath);

        std::wstring fullPath = appDataFolderPath + L"\\" + wideSubfolder;
        std::string narrowPath(fullPath.begin(), fullPath.end());

        return narrowPath;
    }

    return appName;
}