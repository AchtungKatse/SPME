#pragma once

#include "Commands/Display/VertexAttribute.h"
#include "assimp/mesh.h"
namespace SPMEditor {
    class PreviewMesh {
        public:
            friend class PreviewObject;
            PreviewMesh(const aiMesh* mesh);
            PreviewMesh() = default;
            ~PreviewMesh();

            void Draw();

        private:
            static std::vector<VertexAttribute> GetVertexAttributes(const aiMesh* mesh, int& stride);
            static void FlattenVertexArray(void* output, const aiMesh* mesh,  const std::vector<VertexAttribute>& attributes, const int stride);

            const char* m_Name;
            uint m_TextureIndex;

            uint m_VAO;
            uint m_VBO;
            uint m_EBO;
            uint m_IndexCount;
            uint m_VertexCount;
    };
}
