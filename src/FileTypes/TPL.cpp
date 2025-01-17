#include "FileTypes/TPL.h"
#include "IO/FileReader.h"
#include "Types/Types.h"
#include "stb_image_write.h"

namespace SPMEditor {

    TPL::Header TPL::Header::SwapBytes() {
        TPL::Header header = *this;
        ByteSwap4(&header, 3);
        return header;
    }

    TPL::ImageOffset TPL::ImageOffset::SwapBytes() {
        ImageOffset offset = *this;
        ByteSwap4(&offset, 2);
        return offset;
    }

    TPL::PaletteHeader TPL::PaletteHeader::SwapBytes() {
        PaletteHeader header = *this;
        header.entryCount = ByteSwap(header.entryCount);
        ByteSwap((int*)&header + 1, 2);
        return header;
    }

    TPL::ImageHeader TPL::ImageHeader::SwapBytes() {
        ImageHeader header = *this;
        ByteSwap((short*)&header, 2);
        ByteSwap((int*)&header+ 1, 7);
        return header;
    }

    TPL TPL::LoadFromFile(const std::string& path) {
        const std::vector<u8>& data = FileReader::ReadFileBytes(path);
        return LoadFromBytes(data);
    }

    TPL TPL::LoadFromBytes(const std::vector<u8>& data) {
        Assert(data.size() >= sizeof(Header), "Cannot read TPL from data size of {}", data.size());

        // Grab header
        Header header = ((Header*)data.data())->SwapBytes();

        // Get the image offset pointer
        ImageOffset* imageOffsets = (ImageOffset*)(data.data() + header.imageTableOffset);


        TPL tpl;
        tpl.images.resize(header.numImages);
        for (int i = 0; i < header.numImages; i++) {
            // Swap current image offset endianness
            ImageOffset imageOffset = imageOffsets[i].SwapBytes();

            // Get the palette
            PaletteHeader palette;
            if (imageOffset.paletteOffset)
            {
                palette = ((PaletteHeader*)(data.data() + imageOffset.paletteOffset))->SwapBytes();
            }

            // Create an image
            Image image;
            image.header = ((ImageHeader*)(data.data() + imageOffset.headerOffset))->SwapBytes();

            // Read all the blocks
            image.pixels = ReadImage(data.data() + image.header.imageDataAddress, image.header);
            tpl.images[i] = image;
        }

        return tpl;
    }

    void TPL::GetBlockSize(ImageHeader::Format format, int& width, int& height)
    {
        // Get the block size
        width = 4; // Setting values to 4 to save a few switch cases
        height = 4;

        switch (format)
        {
            case ImageHeader::Format::I4:
                {
                    width = 8;
                    height = 8;
                }
            case ImageHeader::Format::I8:
                {
                    width = 8;
                }
            case ImageHeader::Format::IA4:
                {
                    width = 8;
                }
            case ImageHeader::Format::IA8:    break;
            case ImageHeader::Format::RGB565: break;
            case ImageHeader::Format::RGB5A3: break;
            case ImageHeader::Format::RGBA32: break;
            case ImageHeader::Format::C4:
                                              {
                                                  width = 8;
                                                  height = 8;
                                              }
            case ImageHeader::Format::C8:
                                              {
                                                  width = 8;
                                              }
            case ImageHeader::Format::C14X2: break;
            case ImageHeader::Format::CMPR:
                                             {
                                                 width = 8;
                                                 height = 8;
                                             }
        }
    }

    std::vector<Color> TPL::ReadImage(const u8* data, ImageHeader imageHeader)
    {
        int blockWidth;
        int blockHeight;
        GetBlockSize(imageHeader.format, blockWidth, blockHeight);

        // Debugging
        const std::string formatNames[] = {
            "I4",
            "I8",
            "IA4",
            "IA8",
            "RGB565",
            "RGB5A3",
            "RGBA32",
            "Reserved",
            "Reserved",
            "C4",
            "C8",
            "C14X2",
            "Reserved",
            "Reserved",
            "CMPR",
        };

        // Get the number of blocks on each axis
        int numBlocksX = imageHeader.width / blockWidth + (imageHeader.width % blockWidth > 0 ? 1 : 0);
        int numBlocksY = imageHeader.height / blockHeight + (imageHeader.height % blockHeight > 0 ? 1 : 0);

        std::vector<Color> pixels(imageHeader.width * imageHeader.height);
        for (int blockY = 0; blockY < numBlocksY; blockY++) {
            for (int blockX = 0; blockX < numBlocksX; blockX++) {
                const std::vector<Color>& blockColors = ReadBlock(data, imageHeader, blockX * blockWidth, blockY * blockHeight);

                if (imageHeader.format == ImageHeader::Format::RGBA32)
                    data += 64;
                else data += 32;

                for (int y = 0, i = 0; y < blockHeight; y++) {
                    for (int x = 0; x < blockWidth; x++) {
                        // Get the next pixel's global position
                        int xPos = x + blockX * blockWidth;
                        int yPos = y + blockY * blockHeight;
                        int pixelIndex = xPos + yPos * imageHeader.width; // Calculate the pixel index from this

                        if (xPos >= imageHeader.width || yPos >= imageHeader.height)
                            continue;

                        // Read the pixels into the array
                        int blockIndex = x + y * blockWidth;
                        pixels[pixelIndex] = blockColors[i++];

                    }
                }
            }
        }

        return pixels;
    }

    std::vector<Color> TPL::ReadBlock(const u8* data, ImageHeader header, int xPos, int yPos)
    {
        // Special cases for formats that hurt my soul
        switch (header.format)
        {
            default:
                break;
            case ImageHeader::Format::RGBA32: 
                {
                    return ReadRGBA32Block(data);
                }
            case ImageHeader::Format::CMPR:
                {
                    return ReadCMPRBlock(data);
                }
        }

        // Other generic cases
        int blockWidth;
        int blockHeight;
        GetBlockSize(header.format, blockWidth, blockHeight);

        // Go through each block on the x any y axis and read each pixel depending on the format
        // I should probably move the switch to a seperate function, the indents hurt my eyes and soul
        std::vector<Color> pixels;
        // pixels.resize(blockWidth * blockHeight);
        for (int y = 0, i = 0; y < blockHeight; y++) {
            for (int x = 0; x < blockWidth; x++) {
                if (x + xPos >= header.width || y + yPos >= header.height)
                    continue;

                switch (header.format)
                {
                    default:
                        break;

                    case ImageHeader::Format::I4:
                        {
                            u8 val = data[i++];
                            u8 a = (data[val] & 0xf) * 0x11;
                            u8 b = (data[val] >> 4) * 0x11;

                            pixels.push_back(Color(a,a,a,0xff));
                            pixels.push_back(Color(b,b,b,0xff));
                            break;
                        }
                    case ImageHeader::Format::I8:
                        {
                            u8 col = data[i++];

                            pixels.push_back(Color(col,col,col,0xff));
                            break;
                        }
                    case ImageHeader::Format::IA4:
                        {
                            u8 val = data[i++];
                            u8 col = (val & 0xf) * 0x11;
                            u8 alpha = ((val >> 4) & 0xf) * 0x11;

                            pixels.push_back(Color(col,col,col, alpha));
                            break;
                        }
                    case ImageHeader::Format::IA8:
                        {
                            u8 alpha = data[i++];
                            u8 col = data[i++];

                            pixels.push_back(Color(col,col,col, alpha));
                            break;
                        }
                    case ImageHeader::Format::RGB565:
                        {
                            u16 val = ByteSwap(*(u16*)(data + i));
                            u8 r = (val >> 11) * 0x8;
                            u8 g = (val >> 5) * 0x4;
                            u8 b = (val >> 0) * 0x8;

                            pixels.push_back(Color(r,g,b,0xff));
                            i += 2;
                            break;
                        }
                    case ImageHeader::Format::RGB5A3:
                        {
                            u16 val = ByteSwap(*(u16*)(data + i));
                            if ((val >> 0xf) == 0) // has alpha encoding
                            {
                                u8 a = ((val >> 12) & 0x7) * 0x20;
                                u8 r = ((val >> 8) & 0xf) * 0x11;
                                u8 g = ((val >> 4) & 0xf) * 0x11;
                                u8 b = ((val >> 0) & 0xf) * 0x11;

                                pixels.push_back(Color(r,g,b,a));
                            }
                            else // No alpha
                            {
                                u8 r = ((val >> 10) & 0x1f) * 0x8;
                                u8 g = ((val >> 5) & 0x1f) * 0x8;
                                u8 b = ((val >> 0) & 0x1f) * 0x8;

                                pixels.push_back(Color(r,g,b,0xff));
                            }

                            i += 2;
                            break;
                        }
                    case ImageHeader::Format::C4:
                        {
                        }
                    case ImageHeader::Format::C8:
                        {
                        }
                    case ImageHeader::Format::C14X2:
                        {
                        }

                }
            }
        }
        return pixels;
    }

    Color ReadRGB565(u16 val)
    {
        u8 r = (val >> 11) * 0x8;
        u8 g = (val >> 5) * 0x4;
        u8 b = (val >> 0) * 0x8;

        return Color(r,g,b,0xff);
    }

    std::vector<Color> TPL::ReadRGBA32Block(const u8* data)
    {
        std::vector<Color> colors(16);
        for (int i = 0; i < 16; i++) {
            const u8* ptr = data + i * 2;
            colors[i] = Color(ptr[1], ptr[33], ptr[34], ptr[0]);
        }

        return colors;
    }

    std::vector<Color> TPL::ReadCMPRBlock(const u8* data)
    {
        std::vector<Color> pixels(64);
        for (int i = 0; i < pixels.size(); i++) {
           pixels[i] = Color(255, 0, 255, 255); 
        }
        for (int subY = 0, dataOffset = 0; subY < 2; subY++) {
            for (int subX = 0; subX < 2; subX++, dataOffset += 8) {
                // Read the pixels
                u16 aVal = ByteSwap(*(u16*)(data + dataOffset));
                u16 bVal = ByteSwap(*(u16*)(data + dataOffset + 2));
                Color a = ReadRGB565(aVal);
                Color b = ReadRGB565(bVal);

                // Calculate palette
                Color palette[4];
                palette[0] = a;
                palette[1] = b;

                if (aVal > bVal)
                {
                    palette[2] = Color(((a.r << 1) + b.r) / 3,
                                        ((a.g << 1) + b.g) / 3,
                                        ((a.b << 1) + b.b) / 3,
                                        0xff);
                    palette[3] = Color(((b.r << 1) + a.r) / 3,
                                       ((b.g << 1) + a.g) / 3,
                                       ((b.b << 1) + a.b) / 3,
                                         0xff);
                }
                else
                {
                    palette[2] = Color((a.r + b.r) / 2, (a.g + b.g) / 2, (a.b + b.b) / 2, 0xff);
                    palette[3] = Color(0,0,0,0);
                }

                u32 pixelData = ByteSwap(*(u32*)(data + dataOffset + 4));

                for (int i = 0; i < 16; i++) {
                    int index = (pixelData >> (2 * (15 - i))) & 0x3;
                    int x = i % 4;
                    int y = i / 4;
                    int pixelIndex = subX * 4 + x + (subY * 4 + y) * 8;
                    pixels[pixelIndex] = palette[index];
                }
            }
        }

        return pixels;
    }
}
