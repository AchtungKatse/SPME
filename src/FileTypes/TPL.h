#pragma once
#include "PCH.h"

namespace SPMEditor
{
    struct TPL
    {
        private:
            struct Header
            {
                int magic;
                int numImages;
                int imageTableOffset;

                void SwapBytes();
            };

            struct ImageOffset
            {
                int headerOffset;
                int paletteOffset;

                void SwapBytes();
            };

            struct PaletteHeader
            {
                enum struct Format : u32 { IA8 = 0, RGB565 = 1, RGB5A3 = 2};
                u16 entryCount;
                u8 unpacked;
                u8 padding;
                Format format;
                u32 dataOffset;

                void SwapBytes();
            };

        public:
            struct ImageHeader
            {
                enum struct Format: u32 {
                    I4 = 0,
                    I8 = 1,
                    IA4 = 2,
                    IA8 = 3,
                    RGB565 = 4,
                    RGB5A3 = 5,
                    RGBA32 = 6,
                    C4 = 8,
                    C8 = 9,
                    C14X2 = 0xA,
                    CMPR = 0xE,
                };

                u16	height;
                u16	width;
                Format format;
                u32	imageDataAddress;
                u32	wrapS;
                u32	wrapT;
                u32	minFilter;
                u32	magFilter;
                float LODBias;
                u8	edgeLODEnable;
                u8	minLOD;
                u8	maxLOD;
                u8	padding;

                void SwapBytes();
            };

            struct Image
            {
                ImageHeader header;
                string name;
                vector<Color> pixels;
            };

            vector<Image> images;

            static TPL LoadFromFile(string path);
            static TPL LoadFromBytes(vector<u8> data);

        private:
            static void GetBlockSize(ImageHeader::Format format, int& width, int& height);
            static vector<Color> ReadBlock(u8* data, ImageHeader header, int xPos, int yPos);
            static vector<Color> ReadImage(u8* data, ImageHeader imageHeader);
            static vector<Color> ReadCMPRBlock(u8* data);
            static vector<Color> ReadRGBA32Block(u8* data);
    };
}
