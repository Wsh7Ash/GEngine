#include "WebGL2AsyncTexture.h"
#include "../../debug/log.h"
#include "../../platform/VFS.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "stb_image.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/fetch.h>
#endif

namespace ge {
namespace renderer {

WebGL2AsyncTexture::WebGL2AsyncTexture()
    : path_("embedded")
{
}

WebGL2AsyncTexture::WebGL2AsyncTexture(const std::string& path)
    : path_(path)
{
    LoadAsync(path, [this](const TextureLoadResult& result) {
        if (result.success) {
            CreateFromResult(result);
        }
    });
}

WebGL2AsyncTexture::WebGL2AsyncTexture(uint32_t width, uint32_t height, void* data, uint32_t size)
    : width_(width), height_(height)
{
    (void)size;
    glGenTextures(1, &rendererID_);
    glBindTexture(GL_TEXTURE_2D, rendererID_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
}

WebGL2AsyncTexture::~WebGL2AsyncTexture() {
    if (rendererID_) {
        glDeleteTextures(1, &rendererID_);
    }
}

void WebGL2AsyncTexture::Bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, rendererID_);
}

void WebGL2AsyncTexture::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool WebGL2AsyncTexture::Reload() {
    if (path_.empty() || path_ == "embedded") {
        return false;
    }
    
    LoadAsync(path_, [this](const TextureLoadResult& result) {
        if (result.success) {
            CreateFromResult(result);
        }
    });
    return true;
}

void WebGL2AsyncTexture::CreateFromResult(const TextureLoadResult& result) {
    if (!result.success) {
        GE_LOG_ERROR("Failed to create texture: {}", result.errorMessage);
        return;
    }

    if (rendererID_) {
        glDeleteTextures(1, &rendererID_);
    }

    width_ = result.width;
    height_ = result.height;
    channels_ = result.channels;

    glGenTextures(1, &rendererID_);
    glBindTexture(GL_TEXTURE_2D, rendererID_);

    GLenum format = channels_ == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width_, height_, 0, format, GL_UNSIGNED_BYTE, result.data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);

    loading_ = false;
    loadProgress_ = 1.0f;

    GE_LOG_INFO("Async texture loaded: {} ({}x{})", path_, width_, height_);
}

void WebGL2AsyncTexture::LoadAsync(const std::string& path, LoadCallback callback) {
    loading_ = true;
    loadProgress_ = 0.0f;
    path_ = path;

#ifdef __EMSCRIPTEN__
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.onsuccess = [](emscripten_fetch_t* fetch) {
        TextureLoadResult result;
        
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        uint8_t* data = stbi_load_from_memory(
            reinterpret_cast<const uint8_t*>(fetch->data),
            fetch->numBytes,
            &width, &height, &channels, 0);
        
        if (data) {
            result.success = true;
            result.width = static_cast<uint32_t>(width);
            result.height = static_cast<uint32_t>(height);
            result.channels = static_cast<uint32_t>(channels);
            result.data.resize(width * height * channels);
            std::memcpy(result.data.data(), data, result.data.size());
            stbi_image_free(data);
        } else {
            result.success = false;
            result.errorMessage = "Failed to decode image";
        }

        emscripten_fetch_close(fetch);
        
        // In real implementation, we'd call the callback here via a stored callback
        // For now, this demonstrates the pattern
    };
    attr.onerror = [](emscripten_fetch_t* fetch) {
        TextureLoadResult result;
        result.success = false;
        result.errorMessage = "Failed to fetch: " + std::to_string(fetch->status);
        emscripten_fetch_close(fetch);
    };
    attr.timeout = 30000;

    emscripten_fetch(&attr, path.c_str());
#else
    // Fallback for non-Emscripten builds
    std::string data = core::VFS::Get().ReadString(path);
    
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* imageData = stbi_load_from_memory(
        reinterpret_cast<const uint8_t*>(data.data()),
        static_cast<int>(data.size()),
        &width, &height, &channels, 0);

    TextureLoadResult result;
    if (imageData) {
        result.success = true;
        result.width = static_cast<uint32_t>(width);
        result.height = static_cast<uint32_t>(height);
        result.channels = static_cast<uint32_t>(channels);
        result.data.resize(width * height * channels);
        std::memcpy(result.data.data(), imageData, result.data.size());
        stbi_image_free(imageData);
    } else {
        result.success = false;
        result.errorMessage = "Failed to load texture";
    }

    loading_ = false;
    loadProgress_ = 1.0f;
    
    if (callback) {
        callback(result);
    }
#endif
}

void WebGL2AsyncTexture::LoadFromMemoryAsync(std::vector<uint8_t> data, LoadCallback callback) {
    loading_ = true;
    loadProgress_ = 0.5f;

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* imageData = stbi_load_from_memory(
        data.data(),
        static_cast<int>(data.size()),
        &width, &height, &channels, 0);

    TextureLoadResult result;
    if (imageData) {
        result.success = true;
        result.width = static_cast<uint32_t>(width);
        result.height = static_cast<uint32_t>(height);
        result.channels = static_cast<uint32_t>(channels);
        result.data.resize(width * height * channels);
        std::memcpy(result.data.data(), imageData, result.data.size());
        stbi_image_free(imageData);
    } else {
        result.success = false;
        result.errorMessage = "Failed to decode image from memory";
    }

    loading_ = false;
    loadProgress_ = 1.0f;

    if (callback) {
        callback(result);
    }
}

bool CompressedTextureLoader::IsFormatSupported(TextureFormat format) {
    switch (format) {
        case TextureFormat::CompressedETC2:
            return glIsExtensionSupported("GL_EXT_texture_compression_etc2") == GL_TRUE;
        case TextureFormat::CompressedASTC:
            return glIsExtensionSupported("GL_KHR_texture_compression_astc_hdr") == GL_TRUE ||
                   glIsExtensionSupported("GL_EXT_texture_compression_astc_hdr") == GL_TRUE;
        case TextureFormat::CompressedBasis:
            return glIsExtensionSupported("GL_EXT_texture_compression_basis") == GL_TRUE;
        default:
            return true; // Uncompressed formats always supported
    }
}

const char* CompressedTextureLoader::GetFormatExtension(TextureFormat format) {
    switch (format) {
        case TextureFormat::CompressedETC2: return "ktx";
        case TextureFormat::CompressedASTC: return "astc";
        case TextureFormat::CompressedBasis: return "basis";
        default: return "png";
    }
}

void CompressedTextureLoader::DecodeBasis(const std::vector<uint8_t>& data, TextureLoadResult& result) {
    // Basis Universal decoding would require the basisu library
    // This is a placeholder for the full implementation
    result.success = false;
    result.errorMessage = "Basis Universal decoding not implemented - requires basisu library";
}

void CompressedTextureLoader::DecodeKTX2(const std::vector<uint8_t>& data, TextureLoadResult& result) {
    // KTX2 container parsing and transcoding would go here
    // For now, fallback to uncompressed
    result.success = false;
    result.errorMessage = "KTX2 decoding not implemented";
}

bool CompressedTextureLoader::DetectFormat(const std::vector<uint8_t>& data, TextureFormat& outFormat) {
    if (data.size() < 4) {
        outFormat = TextureFormat::Unknown;
        return false;
    }

    // Detect based on magic bytes
    // KTX: "KTX 11" or "KTX 12"
    if (data[0] == 'K' && data[1] == 'T' && data[2] == 'X' && data[3] == ' ') {
        outFormat = TextureFormat::CompressedETC2;
        return true;
    }

    // Basis files have specific magic
    // ETC2/ASTC would be in KTX2 containers
    outFormat = TextureFormat::RGBA8;
    return true;
}

} // namespace renderer
} // namespace ge