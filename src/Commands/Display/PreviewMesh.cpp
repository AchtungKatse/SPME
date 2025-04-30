#include "Commands/Display/PreviewMesh.h"
#include "assimp/color4.h"
#include "glad/glad.h"

namespace SPMEditor {
    PreviewMesh::PreviewMesh(const aiMesh* mesh) : m_Name(0), mTextureIndex(-1), m_VAO(-1), m_VBO(-1), m_EBO(-1), m_IndexCount(0) {
        if (mesh == nullptr)
            return;

        if (mesh->mNumFaces <= 0)
            return; 

        m_Name = mesh->mName.C_Str();

        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glGenBuffers(1, &m_EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

        // ===================
        // Get vertex attributes
        // ===================
        int stride = 0;
        const std::vector<VertexAttribute>& attributes = GetVertexAttributes(mesh, stride);
        char* vertexBuffer = new char[stride * mesh->mNumVertices];

        for (size_t i = 0; i < sizeof(vertexBuffer) / sizeof(float); i++)
            *(float*)(vertexBuffer + sizeof(float) * i) = 12345678.9f;

        FlattenVertexArray(vertexBuffer, mesh, attributes, stride);

        // ===================
        // Get index / vertex count
        // ===================
        m_VertexCount = mesh->mNumVertices;

        m_IndexCount = 0;
        for (uint i = 0; i < mesh->mNumFaces; i++)
            m_IndexCount += mesh->mFaces[i].mNumIndices;
        uint* indexBuffer = new uint[m_IndexCount];

        // ===================================
        // Read indices into one single array
        // ====================================
        for (uint i = 0, offset = 0; i < mesh->mNumFaces; i++)
        {
            memcpy(indexBuffer + offset, mesh->mFaces[i].mIndices, mesh->mFaces[i].mNumIndices * sizeof(unsigned int));
            offset += mesh->mFaces[i].mNumIndices;
        }

        // ===================================
        // Read vertices into one single array
        // ====================================
        glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * stride, vertexBuffer, GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_IndexCount * sizeof(int), indexBuffer, GL_STATIC_DRAW);

        // Free buffers
        delete[] indexBuffer;
        delete[] vertexBuffer;
    }

    std::vector<VertexAttribute> PreviewMesh::GetVertexAttributes(const aiMesh* mesh, int& stride)
    {
        std::vector<VertexAttribute> attributes;
        attributes.clear();

        if (mesh->HasPositions())
        {
            attributes.emplace_back(VertexAttribute(VertexAttribute::POSITION, stride));
            stride += sizeof(Vector3);
        }
        if (mesh->HasNormals())
        {
            attributes.emplace_back(VertexAttribute(VertexAttribute::NORMAL, stride));
            stride += sizeof(Vector3);
        }
        for (int i = 0; i < 4; i++)
        {
            if (mesh->HasVertexColors(i))
            {
                attributes.emplace_back(VertexAttribute((VertexAttribute::Type)(VertexAttribute::COLOR + i), stride));
                stride += sizeof(aiColor4D);
            }
            if (mesh->HasTextureCoords(i))
            {
                attributes.emplace_back(VertexAttribute((VertexAttribute::Type)(VertexAttribute::UV + i), stride));
                stride += sizeof(Vector2);
            }
        }

        return attributes;
    }

    void PreviewMesh::FlattenVertexArray(void* output, const aiMesh* mesh,  const std::vector<VertexAttribute>& attributes, const int stride) {
        for (size_t i = 0; i < attributes.size(); i ++) {
            const VertexAttribute& attr = attributes[i];

            for (uint vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++)
            {

                void* source = (char*)output + vertexIndex * stride + attr.offset;
                switch (attributes[i].type) {
                    case VertexAttribute::NONE:
                        break;
                    case VertexAttribute::POSITION:
                        *((aiVector3D*)source) = mesh->mVertices[vertexIndex];
                        break;
                    case VertexAttribute::NORMAL:
                        *((aiVector3D*)source) = mesh->mNormals[vertexIndex];
                        break;
                    case VertexAttribute::UV:
                        *((aiVector2D*)source) = *(aiVector2D*)&mesh->mTextureCoords[0][vertexIndex];
                        break;
                    case VertexAttribute::UV2:
                        *((aiVector2D*)source) = *(aiVector2D*)&mesh->mTextureCoords[1][vertexIndex];
                        break;
                    case VertexAttribute::UV3:
                        *((aiVector2D*)source) = *(aiVector2D*)&mesh->mTextureCoords[2][vertexIndex];
                        break;
                    case VertexAttribute::UV4:
                        *((aiVector2D*)source) = *(aiVector2D*)&mesh->mTextureCoords[3][vertexIndex];
                        break;
                    case VertexAttribute::COLOR:
                        *((aiColor4D*)source) = *(aiColor4D*)&mesh->mColors[0][vertexIndex];
                        break;
                    case VertexAttribute::COLOR_2:
                        *((aiColor4D*)source) = *(aiColor4D*)&mesh->mColors[1][vertexIndex];
                        break;
                    case VertexAttribute::COLOR_3:
                        *((aiColor4D*)source) = *(aiColor4D*)&mesh->mColors[2][vertexIndex];
                        break;
                    case VertexAttribute::COLOR_4:
                        *((aiColor4D*)source) = *(aiColor4D*)&mesh->mColors[3][vertexIndex];
                        break;
                }
            }

            glVertexAttribPointer(i, attributes[i].GetElementCount(), GL_FLOAT, GL_FALSE, stride, (const void*)(ulong)attributes[i].offset);
            glEnableVertexAttribArray(i);
        }
    }

    PreviewMesh::~PreviewMesh() {
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
        glDeleteVertexArrays(1, &m_VAO);
    }

    void PreviewMesh::Draw() {
        glBindVertexArray(m_VAO); 
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0); 
        glBindVertexArray(0);
    }
}
