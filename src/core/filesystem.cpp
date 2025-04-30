#include "core/filesystem.h"
#include "IO/FileHandle.h"
#include "IO/FileReader.h"
#include "IO/FileWriter.h"
#include <filesystem>

namespace SPMEditor {
    bool filesystem_exists(const char* name) {
        return std::filesystem::exists(name);
    }

    void filesystem_write_file(const char* path, const u8* data, u64 length) {
        SPMEditor::FileWriter::WriteFile(path, data, length);
    }

    FileHandle filesystem_read_file(const char* path) {
        return SPMEditor::FileReader::ReadFileBytes(path);
    }
}
