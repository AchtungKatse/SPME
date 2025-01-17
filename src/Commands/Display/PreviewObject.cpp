#include "Commands/Display/PreviewObject.h"
#include "Commands/Display/ShaderProgram.h"
#include "assimp/material.h"
#include "glm/ext/matrix_transform.hpp"

namespace SPMEditor {

    PreviewObject::PreviewObject(const aiScene* scene, aiNode* node) {
        // Skip rendering colliders
        if (node->mName.length == 1 && node->mName.C_Str()[0] == 'A')
            return;

        // Get transform data (incompatable by default w/ glm)
        node->mTransformation.Decompose(*(aiVector3D*)&m_Scale, *(aiVector3D*)&m_Rotation, *(aiVector3D*)&m_Position);

        m_Meshes.reserve(node->mNumMeshes);
        m_Children.reserve(node->mNumChildren);

        // Load all meshes
        for (int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            PreviewMesh* previewMesh = new PreviewMesh(mesh);

            aiString path;
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            if (mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), path) == AI_SUCCESS)
            {
                std::pair<const aiTexture*, int> textureIndexPair = scene->GetEmbeddedTextureAndIndex(path.C_Str());
                previewMesh->m_TextureIndex = textureIndexPair.second;
            }

            m_Meshes.emplace_back(previewMesh);
        }

        // Then load children recursively
        for (int i = 0; i < node->mNumChildren; i++) {
            m_Children.emplace_back(PreviewObject(scene, node->mChildren[i]));
        }
    }

    void PreviewObject::Draw(ShaderProgram& program, glm::mat4 parentMatrix, PreviewTexture* textures) {
        // Create model matrix
        glm::mat4 translation = glm::translate(glm::mat4(1), m_Position);
        glm::mat4 rotation = glm::rotate(glm::mat4(1), m_Rotation.x, glm::vec3(1, 0, 0));
        rotation = glm::rotate(rotation, m_Rotation.y, glm::vec3(0, 1, 0));
        rotation = glm::rotate(rotation, m_Rotation.z, glm::vec3(0, 0, 1));

        glm::mat4 scale = glm::scale(glm::mat4(1), m_Scale);
        glm::mat4 matrix = parentMatrix * translation * rotation * scale;

        // Set the matrix to render
        program.SetUniformMatrix4fv("model", matrix);
        for (int i = 0; i < m_Meshes.size(); i++) {
            if (m_Meshes[i]->m_TextureIndex != -1)
                textures[m_Meshes[i]->m_TextureIndex].Bind(0);
            m_Meshes[i]->Draw();
        }

        for (int i = 0; i < m_Children.size(); i++) {
            m_Children[i].Draw(program, matrix, textures);
        }
    }

    PreviewObject::~PreviewObject() {
    }
}
