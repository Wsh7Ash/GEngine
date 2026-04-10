#include "OpenGLTexture.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"
#include <glad/glad.h>

// Define STB_IMAGE_IMPLEMENTATION is in stb_image_impl.cpp
#include "../../../../deps/stb/stb_image.h"
#include "../../platform/VFS.h"

namespace ge {
namespace renderer {

    namespace {

    GLenum TextureFilterToGL(TextureFilter filter) {
        switch (filter) {
            case TextureFilter::Nearest: return GL_NEAREST;
            case TextureFilter::Linear: return GL_LINEAR;
            case TextureFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
            case TextureFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
            case TextureFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
            case TextureFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
            default: return GL_LINEAR;
        }
    }

    GLenum TextureWrapToGL(TextureWrap wrap) {
        switch (wrap) {
            case TextureWrap::Repeat: return GL_REPEAT;
            case TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE;
            case TextureWrap::ClampToBorder: return GL_CLAMP_TO_BORDER;
            case TextureWrap::MirrorRepeat: return GL_MIRRORED_REPEAT;
            default: return GL_CLAMP_TO_EDGE;
        }
    }

    } // namespace

    OpenGLTexture::OpenGLTexture(const std::string& path, const TextureSpecification& specification)
        : path_(path), width_(0), height_(0), rendererID_(0), specification_(specification)
    {
        GE_LOG_INFO("Loading texture via VFS: %s", path.c_str());
        
        auto buffer = core::VFS::ReadBinary(path);
        if (buffer.empty())
        {
            GE_LOG_CRITICAL("CRITICAL: Failed to load texture at path: %s", path.c_str());
            std::abort();
        }

        bool isHDR = path.substr(path.find_last_of(".") + 1) == "hdr";
        
        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        
        void* data = nullptr;
        if (isHDR)
            data = stbi_loadf_from_memory(buffer.data(), (int)buffer.size(), &width, &height, &channels, 0);
        else
            data = stbi_load_from_memory(buffer.data(), (int)buffer.size(), &width, &height, &channels, 0);

        if (data)
        {
            width_ = width;
            height_ = height;
            GLenum type = isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE;
            if (isHDR)
            {
                internalFormat_ = GL_RGB16F;
                dataFormat_ = GL_RGB;
            }
            else if (channels == 4)
            {
                internalFormat_ = GL_RGBA8;
                dataFormat_ = GL_RGBA;
            }
            else if (channels == 3)
            {
                internalFormat_ = GL_RGB8;
                dataFormat_ = GL_RGB;
            }

            glCreateTextures(GL_TEXTURE_2D, 1, &rendererID_);
            glTextureStorage2D(rendererID_, 1, internalFormat_, width_, height_);
            ApplySpecification();

            glTextureSubImage2D(rendererID_, 0, 0, 0, width_, height_, dataFormat_, type, data);

            stbi_image_free(data);
        }
        else
        {
            GE_LOG_CRITICAL("CRITICAL: Failed to decode texture at path: %s", path.c_str());
            std::abort();
        }
    }

    OpenGLTexture::OpenGLTexture(uint32_t width, uint32_t height, void* data, uint32_t size, const TextureSpecification& specification)
        : width_(width), height_(height), rendererID_(0), specification_(specification)
    {
        (void)size;
        internalFormat_ = GL_RGBA8;
        dataFormat_ = GL_RGBA;

        glCreateTextures(GL_TEXTURE_2D, 1, &rendererID_);
        glTextureStorage2D(rendererID_, 1, internalFormat_, width_, height_);
        ApplySpecification();

        glTextureSubImage2D(rendererID_, 0, 0, 0, width_, height_, dataFormat_, GL_UNSIGNED_BYTE, data);
    }

    OpenGLTexture::~OpenGLTexture()
    {
        glDeleteTextures(1, &rendererID_);
    }

    void OpenGLTexture::Bind(uint32_t slot) const
    {
        glBindTextureUnit(slot, rendererID_);
    }

    void OpenGLTexture::Unbind() const
    {
        glBindTextureUnit(0, 0);
    }

    void OpenGLTexture::ApplySpecification()
    {
        TextureFilter minFilter = specification_.PixelArt ? TextureFilter::Nearest : specification_.MinFilter;
        TextureFilter magFilter = specification_.PixelArt ? TextureFilter::Nearest : specification_.MagFilter;

        glTextureParameteri(rendererID_, GL_TEXTURE_MIN_FILTER, TextureFilterToGL(minFilter));
        glTextureParameteri(rendererID_, GL_TEXTURE_MAG_FILTER, TextureFilterToGL(magFilter));
        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_S, TextureWrapToGL(specification_.WrapU));
        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_T, TextureWrapToGL(specification_.WrapV));
    }

    bool OpenGLTexture::Reload()
    {
        GE_LOG_INFO("Reloading texture: %s", path_.c_str());
        
        // Load new texture data
        auto buffer = core::VFS::ReadBinary(path_);
        if (buffer.empty())
        {
            GE_LOG_ERROR("Failed to reload texture: %s", path_.c_str());
            return false;
        }

        bool isHDR = path_.substr(path_.find_last_of(".") + 1) == "hdr";
        
        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        
        void* data = nullptr;
        if (isHDR)
            data = stbi_loadf_from_memory(buffer.data(), (int)buffer.size(), &width, &height, &channels, 0);
        else
            data = stbi_load_from_memory(buffer.data(), (int)buffer.size(), &width, &height, &channels, 0);

        if (!data)
        {
            GE_LOG_ERROR("Failed to decode texture during reload: %s", path_.c_str());
            return false;
        }

        // Validate dimensions match
        if (width != width_ || height != height_)
        {
            GE_LOG_ERROR("Texture dimensions changed during reload: %s (%dx%d -> %dx%d)", 
                        path_.c_str(), width_, height_, width, height);
            stbi_image_free(data);
            return false;
        }

        // Validate format matches
        GLenum type = isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE;
        GLenum newInternalFormat = GL_RGB16F;
        GLenum newDataFormat = GL_RGB;
        
        if (!isHDR)
        {
            if (channels == 4)
            {
                newInternalFormat = GL_RGBA8;
                newDataFormat = GL_RGBA;
            }
            else if (channels == 3)
            {
                newInternalFormat = GL_RGB8;
                newDataFormat = GL_RGB;
            }
        }

        if (newInternalFormat != internalFormat_ || newDataFormat != dataFormat_)
        {
            GE_LOG_ERROR("Texture format changed during reload: %s", path_.c_str());
            stbi_image_free(data);
            return false;
        }

        // Update texture data
        glTextureSubImage2D(rendererID_, 0, 0, 0, width_, height_, dataFormat_, type, data);
        stbi_image_free(data);
        
        GE_LOG_INFO("Successfully reloaded texture: %s", path_.c_str());
        return true;
    }

} // namespace renderer
} // namespace ge
