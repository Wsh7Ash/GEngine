#pragma once

#include "../Texture.h"
#include <string>

namespace ge {
namespace renderer {

class WebGL2Texture : public Texture {
public:
    WebGL2Texture(const std::string& path);
    WebGL2Texture(uint32_t width, uint32_t height, void* data, uint32_t size);
    virtual ~WebGL2Texture() override;

    virtual void Bind(uint32_t slot = 0) const override;
    virtual void Unbind() const override;
    virtual uint32_t GetWidth() const override { return width_; }
    virtual uint32_t GetHeight() const override { return height_; }
    virtual uint32_t GetID() const override { return rendererID_; }
    virtual bool operator==(const Texture& other) const override { return rendererID_ == ((WebGL2Texture&)other).rendererID_; }

private:
    std::string path_;
    uint32_t rendererID_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t channels_ = 0;
};

} // namespace renderer
} // namespace ge
