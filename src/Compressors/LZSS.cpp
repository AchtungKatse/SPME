#include "Compressors/LZSS.h"
#include <cstring>

namespace SPMEditor {
    std::vector<u8> LZSS::DecompressBytes(const u8* data, int length) {
        //Decompress LZSS-compressed bytes. Returns a bytearray.//
        u8 type = *data;
        u32 decompressedSize = *(u32*)data >> 8;
        LogInfo("Decompressing lzss stream as type {:x}", type);

        if (type == 0x10)
            return DecompressLzss10(data + 4, length, decompressedSize);
        else if (type == 0x11)
            return DecompressLzss11(data + 4, length, decompressedSize);
        else
            LogInfo("data is not lzss compressed");

        LogError("Failed to find lzss type. Returning empty data.");

        return std::vector<u8>();
    }

    std::vector<u8> LZSS::DecompressLzss10(const u8* indata, int compressedSize, int decompressedSize) {
        // Setup data reading and writing
        std::vector<u8> output(decompressedSize);
        int outPos = 0;

        // Define constants
        const int WindowSize = 4096; /* size of ring buffer */
        const int F = 18; /* upper limit for match_length */
        const int THRESHOLD = 2; /* encode string into position and length
                                    if match_length is greater than this */

        u8 slidingWindow[WindowSize];
        int windowIndex = WindowSize - F;

        int length, disp, k, r, z;
        u8 c;
        int flags;

        r = WindowSize - F;
        flags = 7;
        z = 7;

        const u8* input = indata;
        int inPos = 0;
        while (inPos < compressedSize)
        {
            //ReadBlock();
            flags <<= 1;
            z++;
            if (z == 8)
            {               // read new block flag
                c = input[inPos++];
                flags = c;
                z = 0;              // reset counter
            }
            if ((flags & 0x80) == 0)
            {           // flag bit zero => uncompressed
                c = input[inPos++];
                output[outPos++] = c;

                slidingWindow[r++] = (u8)c;
                r &= (WindowSize - 1);
            }
            else
            {
                length = input[inPos++];
                disp = input[inPos++];

                disp = disp | ((length << 8) & 0xf00);     // match offset
                length = ((length >> 4) & 0x0f) + THRESHOLD;  // match length
                for (k = 0; k <= length; k++)
                {
                    c = slidingWindow[(r - disp - 1) & (WindowSize - 1)];
                    output[outPos++] = c;

                    slidingWindow[r++] = (u8)c;
                    r &= (WindowSize - 1);
                }
            }
        }

        return output;
    }

    std::vector<u8> LZSS::DecompressLzss11(const u8* indata, int compressedSize, int decompressedSize) {
        // Decompress LZSS-compressed bytes. Returns a bytearray.
        std::vector<u8> output(decompressedSize);
        int outputPosition = 0;
        int inputPosition = 0;
        for (int i = 0; i < compressedSize; i++)
        {
            int flags = indata[inputPosition++];
            for (int j = 0; j < 8; j++)
            {
                int flag = (flags >> j) & 1;

                if (flag == 0)
                {
                    output[outputPosition++] = indata[inputPosition++];
                }
                else if (flag == 1)
                {
                    int b = indata[inputPosition++];
                    int indicator = b >> 4;

                    int count = 0;
                    if (indicator == 0)
                    {
                        // 8 bit count, 12 bit disp
                        // indicator is 0, don't need to mask b
                        count = (b << 4);
                        b = indata[inputPosition++];
                        count += b >> 4;
                        count += 0x11;
                    }
                    else if (indicator == 1)
                    {
                        // 16 bit count, 12 bit disp
                        count = ((b & 0xf) << 12) + (indata[inputPosition++] << 4);
                        b = indata[inputPosition++];
                        count += b >> 4;
                        count += 0x111;
                    }
                    else
                    {
                        // indicator is count (4 bits), 12 bit disp
                        count = indicator;
                        count += 1;
                    }


                    int disp = ((b & 0xf) << 8) + indata[inputPosition++];
                    disp += 1;


                    for (int d = 0; d < count; d++)
                        output[outputPosition] = output[outputPosition - disp];
                }
                if (outputPosition >= decompressedSize)
                    break;
            }
            if (outputPosition >= decompressedSize)
                break;
        }

        return output;
    }

    bool Search(const std::vector<u8>& data, int startIndex, int& offset, int& length) {
        length = 0;
        offset = 0;
        // Check backwards for the entire sliding window size
        for (int i = 3; i < 256; i++) {
            // For each character in the sliding window
            if (startIndex - i < 8)
                break;

            for (int k = 0; k < i; k++) {
                u8 previous = data[startIndex - i + k];
                u8 next = data[startIndex + k];

                if (previous != next)
                    break;

                if (k > length) 
                {
                    offset = i;
                    length = k;
                }
            }
        }

        if (startIndex % 0x200 == 0)
            LogInfo("Searching 0x{:x} / 0x{:x} \tLongest Length: 0x{:x}", startIndex, data.size(), length);

        // >2 otherwise no point, same size of bytes
        return length > 2;
    }

    std::vector<u8> LZSS::CompressLzss10(const std::vector<u8>& data) {
        LogInfo("Compressing 0x{:x} bytes of lzss11 data", data.size());
        std::vector<u8> output;
        output.reserve(data.size() + 4);

        output.push_back(0x10);
        output.push_back(0);
        output.push_back(0);
        output.push_back(0);

        *((u32*)output.data()) |= ((u32)data.size() & 0xFFFFFF) << 8; // Writes length


        int outputOffset = 4;
        for (int i = 0; i < data.size();)
        {
            // Go in sets of 8
            u8 flags = 0;
            long flagOffset = output.size();
            output.push_back(0);

            for (int j = 0; j < 8; j++, i++)
            {
                int offset;
                int length;
                /*if (Search(data, i, offset, length))*/
                /*{*/
                /*    flags |= 1 << j;*/
                /*    output.push_back((u8)offset);*/
                /*    output.push_back((u8)length);*/
                /*}*/
                /*else*/
                {
                    output.push_back(data[i]);
                }
            }

            output[flagOffset] = flags;
        }

        return output;
    }
}
