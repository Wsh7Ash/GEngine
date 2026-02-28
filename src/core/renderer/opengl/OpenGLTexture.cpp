#include "OpenGLTexture.h"
#include "../../debug/log.h"
#include "../../debug/assert.h"

// Define STB_IMAGE_IMPLEMENTATION is in stb_image_impl.cpp
#include "../../../../deps/stb/stb_image.h"

namespace ge {
namespace renderer {

    OpenGLTexture::OpenGLTexture(const std::string& path)
        : path_(path), width_(0), height_(0), rendererID_(0)
    {
        GE_LOG_INFO("Loading texture: %s", path.c_str());
        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (data)
        {
            width_ = width;
            height_ = height;

            GLenum internalFormat = 0, dataFormat = 0;
            if (channels == 4)
            {
                internalFormat = GL_RGBA8;
                dataFormat = GL_RGBA;
            }
            else if (channels == 3)
            {
                internalFormat = GL_RGB8;
                dataFormat = GL_RGB;
            }

            internalFormat_ = internalFormat;
            dataFormat_ = dataFormat;

            if (!(internalFormat && dataFormat))
            {
                GE_LOG_CRITICAL("CRITICAL: Texture format not supported for %s", path.c_str());
                std::abort();
            }

            glCreateTextures(GL_TEXTURE_2D, 1, &rendererID_);
            glTextureStorage2D(rendererID_, 1, internalFormat, width_, height_);

            glTextureParameteri(rendererID_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(rendererID_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glTextureSubImage2D(rendererID_, 0, 0, 0, width_, height_, dataFormat, GL_UNSIGNED_BYTE, data);

            stbi_image_free(data);
        }
        else
        {
            GE_LOG_CRITICAL("CRITICAL: Failed to load texture at path: %s", path.c_str());
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
