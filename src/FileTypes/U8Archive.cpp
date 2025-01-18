#include "FileTypes/U8Archive.h"
#include "IO/FileReader.h"
#include "Types/Types.h"
#include <cstring>
#include <filesystem>
#include <unordered_map>
#include "Compressors/LZSS.h"
#include <algorithm>
#include "IO/FileWriter.h"

namespace SPMEditor {

    bool U8Archive::Exists(const std::string& path)
    {
        return rootDirectory.Exists(path);
    }

    U8File& U8Archive::operator[](const std::string& path)
    {
        return rootDirectory[path];
    }

    U8Archive U8Archive::ReadFromFile(const std::string& path, bool compressed)
    {
        const std::vector<u8> data = FileReader::ReadFileBytes(path);
        return ReadFromBytes(data, compressed);
    }

    Directory U8Archive::ReadVirtualDirectory(const u8* data, Node* nodes, int numNodes, int& index, const std::string& path)
    { 
        int stringSectionStart = sizeof(Header) + numNodes * sizeof(Node);

        Node dirNode = nodes[index++];
        ByteSwap2(&dirNode, 2);
        ByteSwap4(&dirNode.dataOffset, 2);

        std::string name = (char*)(data + stringSectionStart + dirNode.nameOffset); 
        Directory dir;
        dir.name = name;

        LogInfo("Reading virt directory '{}' with index '{}'", name , index);

        while (index < dirNode.size)
        {
            // Get the current node and convert to little endian (thanks x86)
            Node node = nodes[index];
            ByteSwap2(&node, 2);
            ByteSwap4(&node.dataOffset, 2);

            // Read the node name
            name = (char*)(data + stringSectionStart + node.nameOffset);

            bool isDirectory = (node.type & 0x100) != 0;
            if (isDirectory)
            {
                std::string nextPath = name;
                if (path.size() > 0) // Added to prevent the path from starting with / and not ./
                    nextPath = path + "/" + name;

                dir.subdirs.emplace_back(ReadVirtualDirectory(data, nodes, numNodes, index, nextPath));
            }
            else // Is a file
            {
                U8File file;
                file.name = name;
                file.data = std::vector<u8>(data + node.dataOffset, data + node.dataOffset + node.size);
                std::string filePath = path + "/" + name;
                dir.files.emplace_back(file);
                index++;
            }
        }

        return dir;
    }

    U8Archive U8Archive::ReadFromBytes(const std::vector<u8>& input, bool compressed)
    {
        static std::vector<u8> decompressed;
        const u8* data = input.data();
        if (compressed)
        {
            decompressed = LZSS::DecompressBytes(input.data(), input.size());
            data = decompressed.data();
        }

        // Check the data is a u8 file
        Assert(*(int*)data == 0x2D38AA55, "Data is not a valid u8 archive. Magic: 0x{:x} != 0x2D38AA55", *(int*)data);

        // Read node table
        int numNodes = ByteSwap(*(int*)(data + 0x28)); // Basically jump to the size of the root node

        Node* nodes = (Node*)(data + sizeof(Header));

        U8Archive archive;
        int index = 0;
        archive.rootDirectory = ReadVirtualDirectory(data, nodes, numNodes, index);
        return archive;
    } 

    bool SortFiles(U8File a, U8File b) {return a.name < b.name;}
    bool SortDirectories(Directory a, Directory b) {return a.name < b.name;}

    void WriteDirectoryNode(u8* data, U8Archive::Node* nodes, Directory& dir, int& nodeIndex, int& dataOffset, int& nameOffset, int& namePosition)
    {
        // Write the current directory
        U8Archive::Node& node = nodes[nodeIndex++];
        node.type = 0x100;
        node.dataOffset = 0;
        node.nameOffset = namePosition;
        node.size = dir.GetTotalNodeCount() + nodeIndex;

        ByteSwap2(&node, 2);
        ByteSwap4(&node.dataOffset, 2);

        strcpy((char*)(data + nameOffset + namePosition), dir.name.c_str());
        namePosition += dir.name.size() + 1;


        sort(dir.files.begin(), dir.files.end(), SortFiles);
        for (auto file : dir.files)
        {
            U8Archive::Node& fileNode = nodes[nodeIndex++];
            fileNode.type = 0;
            fileNode.dataOffset = dataOffset;
            fileNode.nameOffset = namePosition;
            fileNode.size = file.data.size();
            ByteSwap2(&fileNode, 2);
            ByteSwap4(&fileNode.dataOffset, 2);

            strcpy((char*)(data + nameOffset + namePosition), file.name.c_str());
            namePosition += file.name.size() + 1;

            memcpy(data + dataOffset, file.data.data(), file.data.size());

            dataOffset += file.data.size();
            int oldDataOffset = dataOffset;
            dataOffset += 0x40 - (dataOffset - 0x20) % 0x40;
            LogInfo("Rounded data offset from {} to {}", oldDataOffset, dataOffset);
        }

        sort(dir.subdirs.begin(), dir.subdirs.end(), SortDirectories);
        for (auto& subdir : dir.subdirs)
            WriteDirectoryNode(data, nodes, subdir, nodeIndex, dataOffset, nameOffset, namePosition);

    }

    std::vector<u8> U8Archive::CompileU8()
    {
        // Calculate total file size
        int nodeSectionSize = sizeof(U8Archive::Node) * (rootDirectory.GetTotalNodeCount() + 1); // Node size * (num directory + num files + rootNode)

        int headerSize = sizeof(U8Archive::Header) + nodeSectionSize + rootDirectory.GetTotalNameSize();
        int fileSize = headerSize;
        fileSize += 0x40 - (fileSize - 0x20) % 0x40;
        int dataOffset = fileSize;

        fileSize += rootDirectory.GetTotalFileSizePadded();

        std::vector<u8> output(fileSize);
        LogInfo("U8 Total size: {}", fileSize);

        // Create the header
        U8Archive::Header* header = (U8Archive::Header*)(output.data());
        header->tag = U8Archive::Header::U8Tag;
        header->size = nodeSectionSize + rootDirectory.GetTotalNameSize();
        header->rootOffset = 0x20; // Constant
        header->dataOffset = dataOffset;
        ByteSwap4(header, 4);

        int nodeIndex = 0;
        int nameOffset = sizeof(Header) + nodeSectionSize;
        int namePosition = 0;
        WriteDirectoryNode(output.data(), (U8Archive::Node*)(output.data() + sizeof(Header)), rootDirectory, nodeIndex, dataOffset, nameOffset, namePosition);

        // Write all the nodes
        U8Archive::Node* nodes = (U8Archive::Node*)(output.data() + sizeof(U8Archive::Header));
        return output;
    }

    void U8Archive::Dump(const std::string& outputPath)
    {
        // Calculate total file size
        if (rootDirectory.subdirs.size() <= 0 && rootDirectory.files.size() <= 0)
            return;

        std::filesystem::create_directories(outputPath);
        rootDirectory.Dump(outputPath);
        /*for (auto fileNameDataPair : files)*/
        /*{*/
        /*    const std::string path = fileNameDataPair.first;*/
        /*    std::filesystem::path filePath(path);*/
        /**/
        /*    if (filePath.has_parent_path())*/
        /*    {*/
        /*        std::string parentPath = filePath.parent_path().string();*/
        /*        if (parentPath.find("./") != parentPath.npos)*/
        /*            parentPath = parentPath.replace(parentPath.find("./"), 2, "");*/
        /*        std::filesystem::create_directories(outputPath + "/" + parentPath);*/
        /*    }*/
        /**/
        /*    FileWriter::WriteFile(outputPath + "/" + path, fileNameDataPair.second.data);*/
        /*}*/
    }

    Directory LoadDirectory(const std::string& path) {
        std::filesystem::path directoryPath(path);
        Directory dir;
        dir.name = directoryPath.filename().string();

        LogInfo("Loading u8 data from directory '{}'", path);
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!entry.is_regular_file())
                continue;

            std::filesystem::path filePath(entry);
            auto& files = dir.files;
            files.push_back({});
            U8File& file = files[files.size() - 1];
            file.name = filePath.filename().string();
            file.data = FileReader::ReadFileBytes(filePath.string());
            LogInfo("\tFound file '{}'", file.name);
        }

        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!entry.is_directory())
                continue;

            LogInfo("Loading subdir '{}'", entry.path().string());
            auto& subdirs = dir.subdirs;
            subdirs.push_back(LoadDirectory(entry.path().string()));
        }

        return dir;
    }


    bool U8Archive::TryCreateFromDirectory(const std::string& path, U8Archive& output)
    {
        Assert(std::filesystem::is_directory(path), "Trying to create u8 archive but {} is not a directory.", path);
        Directory dot = LoadDirectory(path);
        dot.name = ".";
        output.rootDirectory.subdirs.emplace_back(dot);
        output.rootDirectory.name = "";

        return true;
    }
}
