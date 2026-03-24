#include "OpenGLTexture.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"

// Define STB_IMAGE_IMPLEMENTATION is in stb_image_impl.cpp
#include "../../../../deps/stb/stb_image.h"
#include "../../platform/VFS.h"

namespace ge {
namespace renderer {

    OpenGLTexture::OpenGLTexture(const std::string& path)
        : path_(path), width_(0), height_(0), rendererID_(0)
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

            glTextureParameteri(rendererID_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(rendererID_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTextureSubImage2D(rendererID_, 0, 0, 0, width_, height_, dataFormat_, type, data);

            stbi_image_free(data);
        }
        else
        {
            GE_LOG_CRITICAL("CRITICAL: Failed to decode texture at path: %s", path.c_str());
            std::abort();
        }
    }

    OpenGLTexture::OpenGLTexture(uint32_t width, uint32_t height, void* data, uint32_t size)
        : width_(width), height_(height)
    {
        (void)size;
        internalFormat_ = GL_RGBA8;
        dataFormat_ = GL_RGBA;

        glCreateTextures(GL_TEXTURE_2D, 1, &rendererID_);
        glTextureStorage2D(rendererID_, 1, internalFormat_, width_, height_);

        glTextureParameteri(rendererID_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(rendererID_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_T, GL_REPEAT);

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

} // namespace renderer
} // namespace ge
