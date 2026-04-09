#pragma once

// ================================================================
//  WebGL2AsyncTexture.h
//  Async texture loading for WebGL2/WebAssembly.
// ================================================================

/**
 * @file WebGL2AsyncTexture.h
 * @brief Async texture loading with WebGL2 compatibility.
 * 
 * Features:
 * - Async loading using Emscripten Fetch API
 * - Compressed texture support (Basis Universal, ETC2, ASTC)
 * - Context loss recovery
 * - Progressive loading
 */

#include "../Texture.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

#ifdef __EMSCRIPTEN__
#include <emscripten/fetch.h>
#endif

namespace ge {
namespace renderer {

enum class TextureFormat : uint8_t {
    Unknown = 0,
    RGBA8,
    RGB8,
    CompressedETC2,
    CompressedASTC,
    CompressedBasis,
};

struct TextureLoadResult {
    bool success = false;
    std::string errorMessage;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
    std::vector<uint8_t> data;
    TextureFormat format = TextureFormat::Unknown;
};

class WebGL2AsyncTexture : public Texture {
public:
    using LoadCallback = std::function<void(const TextureLoadResult& result)>;

    WebGL2AsyncTexture();
    WebGL2AsyncTexture(const std::string& path);
    WebGL2AsyncTexture(uint32_t width, uint32_t height, void* data, uint32_t size);
    virtual ~WebGL2AsyncTexture() override;

    virtual void Bind(uint32_t slot = 0) const override;
        virtual void Unbind() const override;

    virtual uint32_t GetWidth() const override { return width_; }
    virtual uint32_t GetHeight() const override { return height_; }
    virtual uint32_t GetID() const override { return rendererID_; }
    virtual bool operator==(const Texture& other) const override { 
        return rendererID_ == static_cast<const WebGL2AsyncTexture&>(other).rendererID_; 
    }
    virtual bool Reload() override;

    static void LoadAsync(const std::string& path, LoadCallback callback);
    static void LoadFromMemoryAsync(std::vector<uint8_t> data, LoadCallback callback);

    bool IsLoading() const { return loading_; }
    float GetLoadProgress() const { return loadProgress_; }

private:
    void CreateFromResult(const TextureLoadResult& result);

    uint32_t rendererID_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t channels_ = 0;

    std::string path_;
    bool loading_ = false;
    float loadProgress_ = 0.0f;
};

class CompressedTextureLoader {
public:
    using LoadCallback = std::function<void(bool success, const std::string& error)>;

    static bool IsFormatSupported(TextureFormat format);
    static const char* GetFormatExtension(TextureFormat format);

    static void DecodeBasis(const std::vector<uint8_t>& data, TextureLoadResult& result);
    static void DecodeKTX2(const std::vector<uint8_t>& data, TextureLoadResult& result);

    static bool DetectFormat(const std::vector<uint8_t>& data, TextureFormat& outFormat);
};

} // namespace renderer
} // namespace ge