#ifndef SPME_NO_VIEWER

#pragma once

#include <GL/gl.h>
#include <GL/glext.h>

namespace SPMEditor 
{
    class PreviewTexture
    {
        public:
            enum class PixelFormat : GLenum
            {
                RGB8 = GL_RGB,
                RGBA8 = GL_RGBA,
                R8 = GL_R8,
            };

            enum class FilterType : GLenum
            {
                Nearest = GL_NEAREST,
                Linear = GL_LINEAR
            };

            enum class WrapType : GLenum
            {
                Repeat = GL_REPEAT,
                ClampToEdge = GL_CLAMP_TO_EDGE,
                ClampToBorder = GL_CLAMP_TO_BORDER,
                MirrorClamp = GL_MIRROR_CLAMP_TO_EDGE,
                MirroredRepeat = GL_MIRRORED_REPEAT,
            };

            PreviewTexture();
            PreviewTexture(const char* path, PixelFormat format = PixelFormat::RGBA8, WrapType wrap = WrapType::Repeat, FilterType filter = FilterType::Linear);
            ~PreviewTexture();

            void Create(const void* data, uint width, uint height, PixelFormat format, WrapType wrap, FilterType filter);
            void Bind(GLuint binding);
            GLuint64 GetTextureHandle();
            /*void MakeResident();*/
            /*void MakeNonResident();*/
            inline const void* GetPixels() const { return m_Pixels; }
            void SetPixels(const void* data, uint width, uint height, uint xOffset, uint yOffset, PixelFormat format, uint level = 0);

            inline uint Width() const  { return m_Width; }
            inline uint Height() const { return m_Height; }

        private:
            /*bool m_IsResident;*/
            /*GLuint64 m_TextureHandle;*/
            GLuint m_TextureID;
            uint m_Width;
            uint m_Height;
            void* m_Pixels;
    };
}


#endif

