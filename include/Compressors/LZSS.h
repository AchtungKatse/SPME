#pragma once

namespace SPMEditor
{
    class LZSS {
        public:
            static std::vector<u8> DecompressBytes(const u8* data, int length);
            static std::vector<u8> CompressLzss10(const std::vector<u8>& data);
            static std::vector<u8> CompressLzss10(u8* data, u64 size);

        private:
            static std::vector<u8> DecompressLzss10(const u8* indata, int compressedSize, int decompressedSize);
            static std::vector<u8> DecompressLzss11(const u8* indata, int compressedSize, int decompressedSize);
    };
}
