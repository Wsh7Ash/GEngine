#include "OpenGLCubemap.h"
#include "../../debug/log.h"
#include "../../../../deps/stb/stb_image.h"
#include "../../platform/VFS.h"

namespace ge {
namespace renderer {

    std::shared_ptr<Cubemap> Cubemap::Create(const std::vector<std::string>& faces)
    {
        return std::make_shared<OpenGLCubemap>(faces);
    }

    std::shared_ptr<Cubemap> Cubemap::Create(uint32_t size, bool hdr)
    {
        return std::make_shared<OpenGLCubemap>(size, hdr);
    }

    OpenGLCubemap::OpenGLCubemap(const std::vector<std::string>& faces)
        : rendererID_(0), size_(0)
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &rendererID_);
        
        int width, height, channels;
        stbi_set_flip_vertically_on_load(0); // Cubemaps usually don't need flip or have specific needs

        for (uint32_t i = 0; i < faces.size(); i++)
        {
            auto buffer = core::VFS::ReadBinary(faces[i]);
            if (buffer.empty()) {
                GE_LOG_ERROR("Failed to load cubemap face: %s", faces[i].c_str());
                continue;
            }

            stbi_uc* data = stbi_load_from_memory(buffer.data(), (int)buffer.size(), &width, &height, &channels, 0);
            if (data)
            {
                if (i == 0) {
                    size_ = width;
                    glTextureStorage2D(rendererID_, 1, GL_RGBA8, size_, size_);
                }

                GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
                glTextureSubImage3D(rendererID_, 0, 0, 0, i, size_, size_, 1, format, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
        }

        glTextureParameteri(rendererID_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(rendererID_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    OpenGLCubemap::OpenGLCubemap(uint32_t size, bool hdr)
        : rendererID_(0), size_(size)
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &rendererID_);
        GLenum internalFormat = hdr ? GL_RGB16F : GL_RGB8;
        glTextureStorage2D(rendererID_, 1, internalFormat, size_, size_);

        glTextureParameteri(rendererID_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(rendererID_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(rendererID_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    OpenGLCubemap::~OpenGLCubemap()
    {
        glDeleteTextures(1, &rendererID_);
    }

    void OpenGLCubemap::Bind(uint32_t slot) const
    {
        glBindTextureUnit(slot, rendererID_);
    }

    void OpenGLCubemap::Unbind() const
    {
        glBindTextureUnit(0, 0);
    }

} // namespace renderer
} // namespace ge
