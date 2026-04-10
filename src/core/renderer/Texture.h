#pragma once

#include "Framebuffer.h"
#include <string>
#include <memory>
#include <cstdint>

namespace ge {
namespace renderer {

    struct TextureSpecification
    {
        TextureFilter MinFilter = TextureFilter::Linear;
        TextureFilter MagFilter = TextureFilter::Linear;
        TextureWrap WrapU = TextureWrap::ClampToEdge;
        TextureWrap WrapV = TextureWrap::ClampToEdge;
        bool PixelArt = false;
    };

    /**
     * @brief Interface for 2D Textures.
     */
    class Texture
    {
    public:
        virtual ~Texture() = default;

        virtual void Bind(uint32_t slot = 0) const = 0;
        virtual void Unbind() const = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetID() const = 0;
        virtual const TextureSpecification& GetSpecification() const = 0;

    virtual bool operator==(const Texture& other) const = 0;

    /**
     * @brief Reloads the texture from its source file.
     * @return True if reload was successful, false otherwise.
     */
    virtual bool Reload() = 0;

    /**
     * @brief Factory method to create a texture from a file.
     */
    static std::shared_ptr<Texture> Create(const std::string& path, bool hdr = false);
    static std::shared_ptr<Texture> Create(const std::string& path, const TextureSpecification& specification, bool hdr = false);
        
        /**
         * @brief Factory method to create a raw texture from memory data.
         */
        static std::shared_ptr<Texture> Create(uint32_t width, uint32_t height, void* data, uint32_t size, bool hdr = false);
        static std::shared_ptr<Texture> Create(uint32_t width, uint32_t height, void* data, uint32_t size, const TextureSpecification& specification, bool hdr = false);
    };

} // namespace renderer
} // namespace ge
