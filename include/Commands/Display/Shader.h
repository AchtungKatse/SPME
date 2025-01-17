#pragma once

namespace SPMEditor
{

    enum class ShaderType : unsigned int
    {
        Fragment = GL_FRAGMENT_SHADER,
        Vertex = GL_VERTEX_SHADER
    };

    class Shader
    {
        friend class ShaderProgram;

        public:
            ~Shader();
            static Shader CreateFromSource(const char* shaderName, const char* shaderSource, ShaderType type);

        /*protected:*/
        /*    Shader(const char* shaderName, const char* filePath, ShaderType type);*/

        private:
            Shader() = default;

            void Create(const char* name, const char* source, ShaderType type);

            unsigned int m_ShaderIndex;
    };
    /**/
    /*class FragmentShader : public Shader*/
    /*{*/
    /*    public:*/
    /*        FragmentShader(const char* shaderName, const char* filePath = nullptr);*/
    /*};*/
    /**/
    /*class VertexShader : public Shader*/
    /*{*/
    /*    public:*/
    /*        VertexShader(const char* shaderName, const char* filePath = nullptr);*/
    /*};*/
}

