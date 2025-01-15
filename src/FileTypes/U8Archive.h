#pragma once
#include "PCH.h"
#include <unordered_map>

namespace SPMEditor
{
    // Thanks Wiibrew! (https://wiibrew.org/wiki/U8_archive)
    struct U8Archive
    {
        public:
            struct File
            {
                string name;
                vector<u8> data;
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

            bool Exists(string path);
            File& operator[](string path);

            void Dump(string path);
            vector<u8> CompileU8();

            unordered_map<string, File> files;

            static U8Archive ReadFromFile(string path, bool compressed = true);
            static U8Archive ReadFromBytes(vector<u8> data, bool compressed = true);
            static bool TryCreateFromDirectory(string path, U8Archive& output);

        private:
            static void ReadVirtualDirectory(unordered_map<string, File>& files, vector<u8> data, Node* nodes, int numNodes, int& index, string path = "");
            static File CreateNodeFromFile(string path);
            static void CreateNodeFromDirectory(string path);
    };
}
