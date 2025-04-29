#pragma once
#include "FileTypes/LevelGeometry/InternalMapFile.h"
#include "assimp/scene.h"

namespace SPMEditor {
    // Todo
    // Add lights
    // Add fog
    // Optimize triangle strips, like at all
    class GeometryExporter {
        public:
            ~GeometryExporter();

            static GeometryExporter* Create();
            void Write(const aiScene* scene, const std::string& path);

        private:
            struct Section {
                Section(const char* name, int offset);

                const char* name;
                u32 offset;
            };

            struct MaterialSubData {
                LevelInternal::Material::SubData data;
                int offset;
            };

            struct ObjectSubData {
                LevelInternal::RawObject::SubData data;
                int offset;
            };

            GeometryExporter();

            // ---------------- /
            // Sections 
            // ---------------- /
            void WriteHeader();
            void WriteInfoSection();
            void WriteMapTextures();
            void WriteSectionTable(std::ofstream& output);
            void WriteMaterials();
            void WriteMeshes();
            void WriteMesh(const aiMesh* mesh);
            void WriteObjects();
            u32 WriteObject(const aiNode* node, u32 parentOffset, u32 previousSibling);
            int GetObjectSize(const aiNode* node);
            void WriteVCDTable();
            void WriteMaterialTable();
            void WriteLightTable();
            void WriteFogTable();
            void WriteTextureTable();
            void WriteAnimationTable();
            void WriteFatPointers();
            void WriteTPL();

            // VCD Table
            void WriteVertexData(u8 vertexScale = 8, u8 uvScale = 8);
            void GetMeshDataRecursive(const aiNode* node);


            /**
             * @brief Adds a string to the string table. If the string was already added to the table, the previous string's offset is returned.
             *
             * @param string the string to be added to the table
             * @return The offset of the string in the table
             */
            int AddString(const char* string);
            /**
             * @brief Writes a pointer to the file and adds the string to the string table.
             *
             * @param string The string to be written.
             * @return The address of the pointer that was written.
             */
            int AppendStringPointer(const char* string);
            template<typename T>
                int _AppendInt(const T value) {
                    int fileOffset = mFileSize;
                    *(T*)(mData + mFileSize + 0x20) = ByteSwap(value);
                    mFileSize += sizeof(T);
                    return fileOffset;
                }

            int AppendBuffer(void* data, int size);
            int AppendPointer(const s32 value);
            int AppendInt32(const s32 value);
            int AppendInt16(const s16 value);
            int AppendInt8(const s8 value);
            int AppendUInt8(const u8 value);
            int AppendVector3(const aiVector3D vector);
            int AppendFloat(const float value);
            int AddPadding(int interval);
            void AddPointer(const s32 address);

            const aiScene* mScene;

            char* mData;
            int mFileSize;
            char* mTextBuffer;
            int mTextBufferSize;

            int mVCDAddress;
            int mRootObjectPtr;
            LevelInternal::RawVCDTable mVCDTable;

            std::vector<int> mStringPointers;
            std::vector<Section> mSections;
            std::vector<int> mVCDPtrs;
            std::vector<ObjectSubData> mObjectSubdatas;
            std::vector<MaterialSubData> mMaterialSubdatas;
            std::vector<int> mPointerList;

            std::vector<int> mTextureNameTable;
            std::map<std::string, int> mTextureNameToIndex;
            std::vector<int> mMeshTable;
            std::vector<LevelInternal::MaterialNameEntry> mMaterialTable;
            std::map<std::string, int> mStringTable;
            std::map<aiVector3D, int> mVertexTable;
            std::map<aiVector3D, int> mUvTable;
            std::map<aiColor4D, int> mColorTable;
            std::map<aiVector3D, int> mNormalTable;
    };
}
