#pragma once
#include "FileTypes/U8File.h"
#include "IO/FileWriter.h"

namespace SPMEditor {
    Directory::Directory() : name(""), files({}), subdirs({}) { }
    U8File& Directory::operator[](const std::string& path)
    {
        // Get root node
        std::string next = path;

        // If it has a directory then a file
        int separator = path.find_first_of('/');
        if (separator != std::string::npos) {
            next = path.substr(0, separator);

            for (int i = 0; i < subdirs.size(); i++) {
                if (subdirs[i].name == next) {
                    return subdirs[i][path.substr(separator + 1)]; // + 1 to remove slash
                }
            }
        }

        // Then its a file in this directory
        for (int i = 0; i < files.size(); i++) {
            if (files[i].name == next) {
                return files[i];
            }
        }

        Assert(false, "Failed to find file at path '{}'", path);
        U8File file;
        return file;
    }

    bool Directory::Exists(const std::string& path) {
        // Get root node
        std::string next = path;

        // If it has a directory then a file
        int separator = path.find_first_of('/');
        if (separator != std::string::npos) {
            next = path.substr(0, separator);

            for (int i = 0; i < subdirs.size(); i++) {
                if (subdirs[i].name == next) {
                    return subdirs[i].Exists(path.substr(separator + 1)); // + 1 to remove slash
                }
            }
        }

        // Then its a file in this directory
        for (int i = 0; i < files.size(); i++) {
            if (files[i].name == next) {
                return true;
            }
        }

        LogError("Failed to find file at path '{}'", path);
        return false;
    }

    void Directory::AddFile(const std::string& path, U8File file) {
        // If it references a directory first
        int separator = path.find_first_of('/');
        if (separator != path.npos)
        {
            const std::string& dirName = path.substr(0, separator);
            const std::string& remainingPath = path.substr(separator + 1);

            for (int i = 0; i < subdirs.size(); i++) {
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
        LogInfo("Dumping '{}' in '{}'", name, outputDir);
        const std::string path = fmt::format("{}/{}", outputDir, name);
        std::filesystem::create_directories(path);

        for (const U8File& file : files) {
            LogInfo("\tDumping '{}' in '{}'", file.name, path);
            FileWriter::WriteFile(fmt::format("{}/{}", path, file.name), file.data, file.size);
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

        for (int i = 0; i < files.size(); i++) {
            fileSize += files[i].size;
            fileSize += 0x40 - (fileSize - 0x20) % 0x40;
        }

        for (int i = 0; i < subdirs.size(); i++) {
            fileSize += subdirs[i].GetTotalFileSizePadded();
        }

        return fileSize;
    }
}
