#pragma once

#include "../Texture.h"
#include <glad/glad.h>

namespace ge {
namespace renderer {

    class OpenGLTexture : public Texture
    {
    public:
        OpenGLTexture(const std::string& path);
        OpenGLTexture(uint32_t width, uint32_t height, void* data, uint32_t size);
        virtual ~OpenGLTexture();

        virtual void Bind(uint32_t slot = 0) const override;
        virtual void Unbind() const override;

        virtual uint32_t GetWidth() const override { return width_; }
        virtual uint32_t GetHeight() const override { return height_; }
        virtual uint32_t GetID() const override { return rendererID_; }

        virtual bool operator==(const Texture& other) const override { return rendererID_ == ((OpenGLTexture&)other).rendererID_; }

    private:
        std::string path_;
        uint32_t width_, height_;
        uint32_t rendererID_;
        GLenum internalFormat_, dataFormat_;
    };

} // namespace renderer
} // namespace ge
