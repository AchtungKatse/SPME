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
    struct Directory
    {
        string name;
        vector<U8Archive::File> files;
        vector<Directory> subdirs;

        U8Archive::File& operator[](string path)
        {
            // Get root node
            string next = path;

            // If it has a directory then a file
            int separator = path.find_first_of('/');
            if (separator != path.npos)
            {
                next = path.substr(0, path.size() - separator);

                for (int i = 0; i < subdirs.size(); i++) {
                    if (subdirs[i].name == next)
                    {
                        return subdirs[i][path.substr(separator)];
                    }
                }
            
            }

            // Then its a file in this directory
            for (int i = 0; i < files.size(); i++) {
               if (files[i].name == next)
               {
                   return files[i];
               }
            }

                cout << "Failed to find file at path '"<<path<<"'"<<endl;
            U8Archive::File file;
            return file;
        }

        void AddFile(string path, U8Archive::File file)
        {
            // If it references a directory first
            int separator = path.find_first_of('/');
            if (separator != path.npos)
            {
                string dirName = path.substr(0, separator);
                string remainingPath = path.substr(separator + 1);

                for (int i = 0; i < subdirs.size(); i++) {
                    // Find the next subdir in the path then tell it to create the file
                    if (subdirs[i].name == dirName)
                    {
                        subdirs[i].AddFile(remainingPath, file);
                        return;
                    }
                }

                // If that dir doesnt exist, imply create 
                Directory dir;
                dir.name = dirName;
                dir.AddFile(remainingPath, file);
                subdirs.push_back(dir);
                return;
            }

            // This is the last bit, add the file
            files.push_back(file);
        }

        int GetTotalFileCount()
        {
            int count = files.size();
            for (auto subdir : subdirs)
                count += subdir.GetTotalFileCount();

            return count;
        }

        int GetTotalNodeCount()
        {
            int count = files.size() + subdirs.size();
            for (auto subdir : subdirs)
                count += subdir.GetTotalNodeCount();
            return count;
        }

        int GetTotalNameSize()
        {
            int size = name.size() + 1;
            for (auto file : files)
            {
                size += file.name.size() + 1;
            }
            
            for (auto subdir : subdirs)
            {
                size += subdir.GetTotalNameSize();
            }

            return size;
        }
    };

    bool U8Archive::Exists(string path)
    {
        return files.find(path) != files.end();
    }

    U8Archive::File& U8Archive::operator[](string path)
    {
        return files[path];
    }

    U8Archive U8Archive::ReadFromFile(string path, bool compressed)
    {
        vector<u8> data = FileReader::ReadFileBytes(path);
        return ReadFromBytes(data);
    }

    void U8Archive::ReadVirtualDirectory(unordered_map<string, File>& files, vector<u8> data, Node* nodes, int numNodes, int& index, string path)
    { 
        int stringSectionStart = sizeof(Header) + numNodes * sizeof(Node);

        Node dirNode = nodes[index++];
        ByteSwap2(&dirNode, 2);
        ByteSwap4(&dirNode.dataOffset, 2);

        string name = (char*)(data.data() + stringSectionStart + dirNode.nameOffset); // C++ is great

        while (index < dirNode.size)
        {
            // Get the current node and convert to little endian (thanks x86)
            Node node = nodes[index];
            ByteSwap2(&node, 2);
            ByteSwap4(&node.dataOffset, 2);

            // Read the node name
            name = (char*)(data.data() + stringSectionStart + node.nameOffset); // C++ is great

            bool isDirectory = (node.type & 0x100) != 0;
            if (isDirectory)
            {
                string nextPath = name;
                if (path.size() > 0) // Added to prevent the path from starting with / and not ./
                    nextPath = path + "/" + name;

                ReadVirtualDirectory(files, data, nodes, numNodes, index, nextPath);
            }
            else // Is a file
            {
                File file;
                file.name = name;
                file.data = vector<u8>(data.data() + node.dataOffset, data.data() + node.dataOffset + node.size);
                string filePath = path + "/" + name;
                files[filePath] = file;
                index++;
            }
        }
    }

    U8Archive U8Archive::ReadFromBytes(vector<u8> data, bool compressed)
    {
        if (compressed)
            data = LZSS::DecompressBytes(data);

        // Check the data is a u8 file
        if (*(int*)data.data() != 0x2D38AA55)
        {
            cout << "Data is not a valid u8 archive." << endl;
            return U8Archive();
        }

        // Read node table
        int numNodes = ByteSwap(*(int*)(data.data() + 0x28)); // Basically jump to the size of the root node

        Node* nodes = (Node*)(data.data() + sizeof(Header));

        U8Archive archive;
        int index = 0;
        ReadVirtualDirectory(archive.files, data, nodes, numNodes, index);
        return archive;
    } 

    Directory CreateDirectory(U8Archive archive, string parentPath, int& totalNodeCount)
    {
        // Do files first
        Directory dir;
        for (auto file : archive.files)
        {
            // Skip over things that are not part of this directory
            filesystem::path path(file.first);
            if (path.parent_path() != parentPath)
                continue;

            // If its a file
            if (path.has_extension())
            {
                dir.files.push_back(file.second);
                totalNodeCount++;
            }
        }
        
        // Then subdirectories
        for (auto file : archive.files)
        {
            // Skip over things that are not part of this directory
            filesystem::path path(file.first);
            if (path.parent_path() != parentPath)
                continue;

            // If its a file
            if (!path.has_extension())
            {
                int childNodeCount = 0;
                dir.subdirs.push_back(CreateDirectory(archive, parentPath, childNodeCount));
                totalNodeCount += childNodeCount;
            }
        }

        return dir;
    }

    Directory CreateRootDirectory(U8Archive archive)
    {
        Directory root;
        Directory local;
        local.name = ".";

        Directory dir;
        for (auto filePair : archive.files)
        {
            dir.AddFile(filePair.first, filePair.second);
        }
        return dir;
    }

    bool SortFiles(U8Archive::File a, U8Archive::File b) {return a.name < b.name;}
    bool SortDirectories(Directory a, Directory b) {return a.name < b.name;}

    void WriteDirectoryNode(u8* data, U8Archive::Node* nodes, Directory& dir, int& nodeIndex, int& dataOffset, int& nameOffset, int& namePosition)
    {
        // Write the current directory
        U8Archive::Node& node = nodes[nodeIndex++];
        node.type = 0x100;
        node.dataOffset = dataOffset;
        node.nameOffset = namePosition;
        node.size = dir.GetTotalNodeCount() + nodeIndex - 1;

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
            cout << "Rounded data offset from " << dec << dataOffset;
            dataOffset += 0x40 - (dataOffset - 0x20) % 0x40;
            cout << " to " << dataOffset <<endl;
        }

        sort(dir.subdirs.begin(), dir.subdirs.end(), SortDirectories);
        for (auto subdir : dir.subdirs)
            WriteDirectoryNode(data, nodes, subdir, nodeIndex, dataOffset, nameOffset, namePosition);

    }

    vector<u8> U8Archive::CompileU8()
    {
        // Calculate total file size
        Directory root = CreateRootDirectory(*this);

        int nodeSectionSize = sizeof(U8Archive::Node) * (root.GetTotalNodeCount() + 1); // Node size * (num directory + num files + rootNode)

        int headerSize = sizeof(U8Archive::Header) + nodeSectionSize + root.GetTotalNameSize();
        int fileSize = headerSize;
        fileSize += 0x40 - (fileSize - 0x20) % 0x40;
        int dataOffset = fileSize;

        for (auto file : files)
        {
            fileSize += file.second.data.size();
            fileSize += 0x40 - (fileSize - 0x20) % 0x40;
        }

        vector<u8> output(fileSize);
        cout << "U8 Total size: " << dec << fileSize << endl;

        // Create the header
        U8Archive::Header* header = (U8Archive::Header*)(output.data());
        header->tag = U8Archive::Header::U8Tag;
        header->size = nodeSectionSize + root.GetTotalNameSize();
        header->rootOffset = 0x20; // Constant
        header->dataOffset = dataOffset;
        ByteSwap4(header, 4);

        int nodeIndex = 0;
        int nameOffset = sizeof(Header) + nodeSectionSize;
        int namePosition = 0;
        WriteDirectoryNode(output.data(), (U8Archive::Node*)(output.data() + sizeof(Header)), root, nodeIndex, dataOffset, nameOffset, namePosition);

        // Write all the nodes
        U8Archive::Node* nodes = (U8Archive::Node*)(output.data() + sizeof(U8Archive::Header));
        return output;
    }

    void U8Archive::Dump(string outputPath)
    {
        // Calculate total file size
        if (files.size() <= 0)
            return;
        filesystem::create_directories(outputPath);

        for (auto pair : files)
        {
            string path = pair.first;
            filesystem::path filePath(path);

            if (filePath.has_parent_path())
            {
                string parentPath = filePath.parent_path().string();
                if (parentPath.find("./") != parentPath.npos)
                    parentPath = parentPath.replace(parentPath.find("./"), 2, "");
                filesystem::create_directories(outputPath + "/" + parentPath);
            }

            FileWriter::WriteFile(outputPath + "/" + path, pair.second.data);
        }
    }


    bool U8Archive::TryCreateFromDirectory(string path, U8Archive& output)
    {
        if (!filesystem::is_directory(path))
        {
            cout << "Trying to create u8 archive from invalid directory: '" << path << "'" << endl;
            output = U8Archive();
            return false;
        }

        for (const auto entry : filesystem::recursive_directory_iterator(path))
        {
            if (!entry.is_regular_file())
                continue;

            File file;
            filesystem::path filePath = filesystem::path(entry);
            file.name = filePath.filename();
            vector<u8> data = FileReader::ReadFileBytes(filePath);
            file.data = vector<u8>(data.size());
            for (int i = 0; i < data.size(); i++) {
                file.data[i] = data[i]; 
            }

            string key = filePath.string().replace(0, path.size(), ".");
            output.files[key] = file;
        }

        return true;
    }
}
