#pragma once
#include <unordered_map>

namespace SPMEditor
{
    // Thanks Wiibrew (https://wiibrew.org/wiki/U8_archive)
    struct U8Archive
    {
        public:
            struct File
            {
                std::string name;
                std::vector<u8> data;
            };

            struct Header 
            {
                static const u32 U8Tag = 0x55AA382D;
                u32 tag; // 0x55AA382D "U.8-"
                u32 rootOffset; // offset to root_node, always 0x20.
                u32 size; // size of header from root_node to end of string table.
                u32 dataOffset; // offset to data -- this is rootnode_offset + header_size, aligned to 0x40.
                u8 padding[16];
            };

            struct Node 
            {
                u16 type; //this is really a u8
                u16 nameOffset; //really a "u24"
                u32 dataOffset;
                u32 size;
            };


        public:

            bool Exists(const std::string& path);
            File& operator[](const std::string& path);

            void Dump(const std::string& path);
            std::vector<u8> CompileU8();

            std::unordered_map<std::string, File> files;

            static U8Archive ReadFromFile(const std::string& path, bool compressed);
            static U8Archive ReadFromBytes(const std::vector<u8>& data, bool compressed);
            static bool TryCreateFromDirectory(const std::string& path, U8Archive& output);

        private:
            static void ReadVirtualDirectory(std::unordered_map<std::string, File>& files, const u8* data, Node* nodes, int numNodes, int& index, const std::string& path = "");
            static File CreateNodeFromFile(const std::string& path);
            static void CreateNodeFromDirectory(const std::string& path);
    };
}
