#include "FileTypes/U8File.h"
#include "IO/FileWriter.h"
#include <cstdio>

namespace SPMEditor {
    Directory::Directory() : name(""), files({}), subdirs({}) { }
    bool Directory::Get(const std::string& path, U8File** outFile)
    {
        // Get root node
        std::string next = path;

        // If it has a directory then a file
        size_t separator = path.find_first_of('/');
        if (separator != std::string::npos) {
            next = path.substr(0, separator);

            for (size_t i = 0; i < subdirs.size(); i++) {
                if (subdirs[i].name == next) {
                    return subdirs[i].Get(path.substr(separator + 1), outFile); // + 1 to remove slash
                }
            }
        }

        // Then its a file in this directory
        for (size_t i = 0; i < files.size(); i++) {
            if (files[i].name == next) {
                *outFile = &files[i];
                return true;
            }
        }

        Assert(false, "U8Archive.Get() failed to find file at path '%s'", path.c_str());
        outFile = nullptr;
        return false;
    }

    bool Directory::Exists(const std::string& path) {
        // Get root node
        std::string next = path;

        // If it has a directory then a file
        size_t separator = path.find_first_of('/');
        if (separator != std::string::npos) {
            next = path.substr(0, separator);

            for (size_t i = 0; i < subdirs.size(); i++) {
                if (subdirs[i].name == next) {
                    return subdirs[i].Exists(path.substr(separator + 1)); // + 1 to remove slash
                }
            }
        }

        // Then its a file in this directory
        for (size_t i = 0; i < files.size(); i++) {
            if (files[i].name == next) {
                return true;
            }
        }

        LogError("Failed to find file at path '%s'", path.c_str());
        return false;
    }

    void Directory::AddFile(const std::string& path, U8File file) {
        // If it references a directory first
        size_t separator = path.find_first_of('/');
        if (separator != path.npos)
        {
            const std::string& dirName = path.substr(0, separator);
            const std::string& remainingPath = path.substr(separator + 1);

            for (size_t i = 0; i < subdirs.size(); i++) {
                // Find the next subdir in the path then tell it to create the file
                if (subdirs[i].name == dirName)
                {
                    subdirs[i].AddFile(remainingPath, file);
                    return;
                }
            }

            // If that dir doesnt exist, imply create 
            Directory dir;
            dir.name = dirName;
            dir.AddFile(remainingPath, file);
            subdirs.push_back(dir);
            return;
        }

        // This is the last bit, add the file
        files.push_back(file);
    }

    void Directory::Dump(const std::string& outputDir) const {
        LogInfo("Dumping '%s' in '%s'", name.c_str(), outputDir.c_str());
        char path[0x400] = {};
        // NOTE: This function uses snprintf which is technically insecure, but should be fine in this use case
        snprintf(path, sizeof(path), "%s/%s", outputDir.c_str(), name.c_str());
        std::filesystem::create_directories(path);

        for (const U8File& file : files) {
            LogInfo("\tDumping '%s' in '%s'", file.name.c_str(), path);
            char filePath[0x500] = {};
            snprintf(filePath, sizeof(filePath), "%s/%s", path, file.name.c_str());
            FileWriter::WriteFile(filePath, file.data, file.size);
        }

        for (const Directory& subdir : subdirs) {
            subdir.Dump(path);
        }
    }

    int Directory::GetTotalFileCount() const {
        int count = files.size();
        for (const auto& subdir : subdirs)
            count += subdir.GetTotalFileCount();

        return count;
    }

    int Directory::GetTotalNodeCount() const {
        int count = files.size() + subdirs.size();
        for (const auto& subdir : subdirs)
            count += subdir.GetTotalNodeCount();
        return count;
    }

    int Directory::GetTotalNameSize() const {
        int size = name.size() + 1;
        for (const auto& file : files)
        {
            size += file.name.size() + 1;
        }

        for (const auto& subdir : subdirs)
        {
            size += subdir.GetTotalNameSize();
        }

        return size;
    }

    int Directory::GetTotalFileSizePadded() const {
        int fileSize = 0;

        for (size_t i = 0; i < files.size(); i++) {
            fileSize += files[i].size;
            fileSize += 0x40 - (fileSize - 0x20) % 0x40;
        }

        for (size_t i = 0; i < subdirs.size(); i++) {
            fileSize += subdirs[i].GetTotalFileSizePadded();
        }

        return fileSize;
    }
}
