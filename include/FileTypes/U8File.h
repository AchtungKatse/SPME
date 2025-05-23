#pragma once

#include <string>
#include <vector>
namespace SPMEditor {
    struct U8File
    {
        std::string name;
        u8* data;
        u64 size;
    };

    struct Directory
    {
        public:
            Directory();

            std::string name;
            std::vector<U8File> files;
            std::vector<Directory> subdirs;

            bool Get(const std::string& path, U8File** outFile);
            bool Exists(const std::string& path);
            void AddFile(const std::string& path, U8File file);
            void Dump(const std::string& outputDir) const;
            int GetTotalFileCount() const;
            int GetTotalNodeCount() const;
            int GetTotalNameSize() const;
            int GetTotalFileSizePadded() const;
    };
}
