#pragma once

#include "../Texture.h"

namespace ge {
namespace renderer {

    /**
     * @brief Placeholder for DX11 Texture implementation.
     */
    class DX11Texture : public Texture
    {
    public:
        DX11Texture(const std::string& path);
        DX11Texture(uint32_t width, uint32_t height, void* data, uint32_t size);
        virtual ~DX11Texture();

        virtual void Bind(uint32_t slot = 0) const override;
        virtual void Unbind() const override;

        virtual uint32_t GetWidth() const override { return width_; }
        virtual uint32_t GetHeight() const override { return height_; }
        virtual uint32_t GetID() const override { return 0; }

    private:
        uint32_t width_, height_;
    };

} // namespace renderer
} // namespace ge
