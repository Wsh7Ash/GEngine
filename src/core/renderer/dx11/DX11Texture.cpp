#include "DX11Texture.h"
#include "../../debug/log.h"

namespace ge {
namespace renderer {

    DX11Texture::DX11Texture(const std::string& path)
        : width_(0), height_(0)
    {
        GE_LOG_INFO("DX11Texture loading (Placeholder): %s", path.c_str());
    }

    DX11Texture::DX11Texture(uint32_t width, uint32_t height, void* data, uint32_t size)
        : width_(width), height_(height)
    {
        (void)data; (void)size;
        GE_LOG_INFO("DX11Texture creation from memory (Placeholder)");
    }

    DX11Texture::~DX11Texture() {}

    void DX11Texture::Bind(uint32_t slot) const { (void)slot; }
    void DX11Texture::Unbind() const {}

} // namespace renderer
} // namespace ge
