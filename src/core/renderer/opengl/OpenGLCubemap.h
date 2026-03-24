#pragma once

#include "../Cubemap.h"
#include <glad/glad.h>

namespace ge {
namespace renderer {

    class OpenGLCubemap : public Cubemap
    {
    public:
        OpenGLCubemap(const std::vector<std::string>& faces);
        OpenGLCubemap(uint32_t size, bool hdr = false);
        virtual ~OpenGLCubemap();

        virtual void Bind(uint32_t slot = 0) const override;
        virtual void Unbind() const override;
        virtual uint32_t GetID() const override { return rendererID_; }

    private:
        uint32_t rendererID_;
        uint32_t size_;
    };

} // namespace renderer
} // namespace ge
