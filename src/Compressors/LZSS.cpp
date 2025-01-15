#include "Compressors/LZSS.h"
#include "PCH.h"
#include <cstring>

using namespace std;
using namespace SPMEditor;

vector<u8> DecompressBytes(u8* data, int length);
vector<u8> DecompressLzss10(u8* indata, int compressedSize, int decompressedSize);
vector<u8> DecompressRawLzss11(u8* indata, int compressedSize, int decompressedSize);

vector<u8> LZSS::DecompressBytes(vector<u8> input)
{
    cout << "Type: " << hex << (int)input[0] << endl;
    return DecompressBytes(input.data(), input.size());
}

vector<u8> LZSS::DecompressBytes(u8* data, int length)
{
    //Decompress LZSS-compressed bytes. Returns a bytearray.//
    u8 type = *data;
    u32 decompressedSize = (*(u32*)data) >> 8;

    if (type == 0x10)
        return DecompressLzss10(data + 4, length, decompressedSize);
    else if (type == 0x11)
        return DecompressRawLzss11(data + 4, length, decompressedSize);
    else
        cout << "data is not lzss compressed" << endl;
    cout << "Type: " << hex << (int)type << endl;


    return vector<u8>();
}

vector<u8> DecompressLzss10(u8* indata, int compressedSize, int decompressedSize)
{
    // Setup data reading and writing
    vector<u8> output(decompressedSize);
    int outPos = 0;

    // Define constants
    const int WindowSize = 4096; /* size of ring buffer */
    const int F = 18; /* upper limit for match_length */
    const int THRESHOLD = 2; /* encode string into position and length
                                if match_length is greater than this */

    u8 slidingWindow[WindowSize];
    int windowIndex = WindowSize - F;

    int i, j, k, r, z;
    u8 c;
    int flags;

    r = WindowSize - F;
    flags = 7;
    z = 7;

    u8* input = indata;
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
            i = input[inPos++];
            j = input[inPos++];

            j = j | ((i << 8) & 0xf00);     // match offset
            i = ((i >> 4) & 0x0f) + THRESHOLD;  // match length
            for (k = 0; k <= i; k++)
            {
                c = slidingWindow[(r - j - 1) & (WindowSize - 1)];
                output[outPos++] = c;

                slidingWindow[r++] = (u8)c;
                r &= (WindowSize - 1);
            }
        }
    }

    return output;
}

vector<u8> DecompressRawLzss11(u8* indata, int compressedSize, int decompressedSize)
{
    // Decompress LZSS-compressed bytes. Returns a bytearray.
    vector<u8> output(decompressedSize);
    for (int i = 0; i < decompressedSize; i++) {
       output[i] = 255; 
    }
    int outputPosition = 0;
    int inputPosition = 0;
    for (int i = 0; i < decompressedSize; i++)
    {
        int b = indata[inputPosition++];
        //int[] flags = bits(b);
        for (int j = 0; j < 8; j++)
        {
            int flag = (b >> j) & 1;

            if (flag == 0)
            {
                output[outputPosition++] = indata[inputPosition++];
            }
            else if (flag == 1)
            {
                b = indata[inputPosition++];
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

bool Search(vector<u8> data, int startIndex, int& offset, int& length)
{

    int longestLength = 0;
    offset = -1;
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
    length = longestLength;
    if (startIndex % 0x200 == 0)
    cout << "Searching 0x" << hex << startIndex << " / 0x" << data.size() << " \tLongest Length: 0x" << hex << length << endl;

    // >2 otherwise no point, same size of bytes
    return length > 2;
}

vector<u8> LZSS::CompressLzss11(vector<u8> data)
{
    cout << "Compressing data" << endl;
    vector<u8> output;
    output.push_back(0x11);
    
    output.push_back(0);
    output.push_back(0);
    output.push_back(0);

    *((u32*)output.data()) |= ByteSwap((u32)data.size());

    output.reserve(data.size());

    for (int i = 0; i < data.size(); i++)
    {
        // Go in sets of 8
        int flags = 0;
        long flagOffset = output.size();
        output.push_back(0);

        for (int j = 0; j < 8; j++, i++)
        {
            int offset;
            int length;
            // if (Search(data, i, offset, length))
            // {
            //     flags |= 1 << j;
            //     output.push_back(offset);
            //     output.push_back(length);
            // }
            // else
            {
                output.push_back(data[i]);
            }
        }

        output[flagOffset] = flags;
    }

    return output;
}

