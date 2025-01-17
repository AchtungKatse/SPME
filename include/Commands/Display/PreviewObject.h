#pragma once
#include "Commands/Display/PreviewMesh.h"
#include "Commands/Display/PreviewTexture.h"
#include "Commands/Display/ShaderProgram.h"
#include "assimp/mesh.h"
#include "assimp/scene.h"

namespace SPMEditor {
    class PreviewObject {
        public:
            ~PreviewObject();
            PreviewObject() = default;
            PreviewObject(const aiScene* scene, aiNode* mesh);

            void Draw(ShaderProgram& program, glm::mat4 parentMatrix, PreviewTexture* textures);

        private:
            std::vector<PreviewObject> m_Children;
            std::vector<PreviewMesh*> m_Meshes;

            glm::vec3 m_Position;
            glm::vec3 m_Rotation;
            glm::vec3 m_Scale;
    };
}
