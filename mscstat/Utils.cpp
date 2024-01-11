#include "Utils.h"
#include "Application.h"

bool Utils::ExtractZip(std::string source, std::string destination) {
       zip_t* archive = zip_open(source.c_str(), ZIP_RDONLY, nullptr);
       if (!archive) {
            std::cerr << "Failed to open the zip file." << std::endl;
            return false;
        }

        zip_int64_t numEntries = zip_get_num_entries(archive, 0);
        if (numEntries <= 0) {
            LogManager::GetInstance().LogError("No entries found in the zip file.");
            zip_close(archive);
            return false;
        }

        for (zip_int64_t i = 0; i < numEntries; ++i) {
            zip_stat_t entryStat;
            if (zip_stat_index(archive, i, 0, &entryStat) != 0) {
                LogManager::GetInstance().LogError("Failed to get stat for entry: {0}", i);
                zip_close(archive);
                return false;
            }

            std::string entryName = entryStat.name;
            std::string outputPath = destination + "/" + entryName;
            // Create directories if they don't exist
            std::filesystem::path outputDir(outputPath);
            std::filesystem::create_directories(outputDir.parent_path());

            zip_file_t* file = zip_fopen_index(archive, i, 0);
            if (!file) {
                LogManager::GetInstance().LogError("Failed to open file: {0}", entryName);
                zip_close(archive);
                return false;
            }

            std::ofstream outputFile(outputPath, std::ios::binary);
            if (!outputFile.is_open()) {
                LogManager::GetInstance().LogError("Failed to create output file: {0}", outputPath);
                zip_fclose(file);
                zip_close(archive);
                return false;
            }

            char buffer[1024];
            zip_int64_t bytesRead;
            while ((bytesRead = zip_fread(file, buffer, sizeof(buffer))) > 0) {
                outputFile.write(buffer, bytesRead);
            }

            outputFile.close();
            zip_fclose(file);
        }

        zip_close(archive);

        LogManager::GetInstance().LogInfo("Zip file successfully extracted to: {0}", destination);
        return true;
}

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