#pragma once

#include "../Texture.h"
#include <glad/glad.h>

namespace ge {
namespace renderer {

    class OpenGLTexture : public Texture
    {
    public:
        OpenGLTexture(const std::string& path, const TextureSpecification& specification = {});
        OpenGLTexture(uint32_t width, uint32_t height, void* data, uint32_t size, const TextureSpecification& specification = {});
        virtual ~OpenGLTexture();

        virtual void Bind(uint32_t slot = 0) const override;
        virtual void Unbind() const override;

        virtual uint32_t GetWidth() const override { return width_; }
        virtual uint32_t GetHeight() const override { return height_; }
        virtual uint32_t GetID() const override { return rendererID_; }
        virtual const TextureSpecification& GetSpecification() const override { return specification_; }

        virtual bool operator==(const Texture& other) const override { return rendererID_ == ((OpenGLTexture&)other).rendererID_; }

        virtual bool Reload() override;

    private:
        void ApplySpecification();

        std::string path_;
        uint32_t width_, height_;
        uint32_t rendererID_;
        GLenum internalFormat_, dataFormat_;
        TextureSpecification specification_;
    };

} // namespace renderer
} // namespace ge
