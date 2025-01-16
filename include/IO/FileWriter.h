#pragma once

namespace SPMEditor::FileWriter {
    void WriteFile(const std::string& path, const std::vector<u8>& data);
    void WriteFile(const std::string& path, const u8* data, int length);
}
