#include "Compressors/LZSS.h"
#include "IO/FileWriter.h"
#include "UnitTests/LZSSTests.h"
#include <cstdlib>
#include <vector>

namespace SPMEditor::Testing { 
    
    bool TestLZSSCompression() {
        std::vector<u8> data(1024);

        for (int i = 64; i < data.size(); i++) {
            data[i] = rand() % 255;
        }

        std::vector<u8> compressed = LZSS::CompressLzss10(data);

        FileWriter::WriteFile("LZSS Original.bin", data);
        FileWriter::WriteFile("LZSS Compressed.bin", compressed);
        std::vector<u8> decompressed = LZSS::DecompressBytes(compressed.data(), compressed.size());
        FileWriter::WriteFile("LZSS Decompressed.bin", decompressed);

        if (data.size() != decompressed.size())
        {
            LogError("Decompressed size does not match original size. Got {}, expected {}", decompressed.size(), compressed.size());
            return false;
        }

        if (memcmp(data.data(), decompressed.data(), data.size()) != 0)
        {
            for(int i = 0; i < 100; i++) {
                LogError("{:3x} : {:3x}", data[i], decompressed[i]);
            }
            return false;
        }

        return true;
    }
}
