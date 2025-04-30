#include "Commands/Display/PreviewTexture.h"
#include "stb_image.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <array>
#include <filesystem>
#include "stb_image.h"

namespace SPMEditor 
{
    PreviewTexture::PreviewTexture() : 
        m_TextureID(0),
        m_Width(0),
        m_Height(0),
        m_Pixels(nullptr)
    { }

    PreviewTexture::PreviewTexture(const char* filePath, PixelFormat format, WrapType wrap, FilterType filter) :
        m_TextureID(0),
        m_Pixels(nullptr)
    { 

        // Check if file path exists
        if (!std::filesystem::exists(filePath))
        {
            LogError("Failed to open texture '%s'", filePath);
            constexpr std::array<unsigned char, 3> DefaultColor = {255, 0, 255};
            Create(DefaultColor.data(), 1, 1, format, wrap, filter);
            return;
        }

        // Otherwise, read the image data and create the image
        int numChannels;
        switch (format)
        {
            case PixelFormat::RGB8: numChannels = STBI_rgb; break;
            case PixelFormat::RGBA8: numChannels = STBI_rgb_alpha; break;
            case PixelFormat::R8: numChannels = STBI_grey; break;
        }

        int width, height, channels = 0;
        m_Pixels = stbi_load(filePath, &width, &height, &channels, numChannels);
        Create(m_Pixels, width, height, format, wrap, filter);
    }

    PreviewTexture::~PreviewTexture() {
        /*if (m_TextureID != 0)*/
        /*    glDeleteTextures(1, &m_TextureID);*/

        /*if (m_Pixels)*/
        /*    stbi_image_free(m_Pixels);*/
    }

    void PreviewTexture::Create(const void* data, uint width, uint height, PixelFormat format, WrapType wrap, FilterType filter) {
        m_Width = width;
        m_Height = height;

        m_TextureID = 0;
        glGenTextures(1, &m_TextureID);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        Assert(m_TextureID != 0, "Failed to generate texture: %d", m_TextureID);

        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLenum)wrap);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLenum)wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLenum)filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLenum)filter);
        //
        // Create the texture
        glTexImage2D(GL_TEXTURE_2D, 0, (GLenum)format, width, height, 0, (GLenum)format, GL_UNSIGNED_BYTE, data);
    }

    void PreviewTexture::Bind(GLuint binding)
    {
        /*CoreAssert(!m_IsResident, "Trying to bind texture to uniform when it is resident to the GPU (call Texture::ReleaseARB)");*/
        Assert(binding >= 0 && binding < 32, "Binding is out of bounds. 0 <= %u < 32", binding);

        glActiveTexture(GL_TEXTURE0 + binding);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
    }

    /*GLuint64 Texture::GetTextureHandle()*/
    /*{*/
    /*    if (m_TextureHandle != 0)*/
    /*        return m_TextureHandle;*/
    /**/
    /*    m_TextureHandle = glGetTextureHandleARB(m_TextureID);*/
    /*    CoreAssert(m_TextureHandle > 0, "Failed to get texture handle: %ul", m_TextureHandle);*/
    /**/
    /*    return m_TextureHandle;*/
    /*}*/
    /**/
    /*void Texture::MakeResident()*/
    /*{*/
    /*    m_IsResident = true;*/
    /*    GLuint64 handle = GetTextureHandle();*/
    /*    glMakeImageHandleResidentARB(handle, GL_READ_ONLY);*/
    /*}*/
    /**/
    /*void Texture::MakeNonResident()*/
    /*{*/
    /*    m_IsResident = false;*/
    /*    glad_glMakeTextureHandleNonResidentARB(GetTextureHandle());*/
    /*}*/
    /**/
    void PreviewTexture::SetPixels(const void* data, uint width, uint height, uint xOffset, uint yOffset, PixelFormat format, uint level)
    {
        Assert(m_TextureID != 0, "Trying to set texture data without having created it");
        Assert(width + xOffset <= m_Width, "Trying to set pixels out of bounds of texture, %u >= %u", width + xOffset, m_Width);
        Assert(height + yOffset <= m_Height, "Trying to set pixels out of bounds of texture, %u >= %u", height + yOffset, m_Height);

        glTextureSubImage2D(m_TextureID, level, xOffset, yOffset, width, height, (GLuint)format, GL_UNSIGNED_BYTE, data);
    }
}

