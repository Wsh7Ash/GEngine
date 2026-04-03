#pragma once

// ================================================================
//  GlyphCache.h
//  Glyph texture atlas caching for efficient text rendering.
// ================================================================

#include "FontManager.h"
#include "../renderer/Texture.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace ge {
namespace renderer {

struct GlyphSlot {
    GlyphMetrics metrics;
    std::vector<uint8_t> bitmap;
    bool isCached = false;
};

class GlyphCache {
public:
    explicit GlyphCache(uint32_t atlasWidth = 512, uint32_t atlasHeight = 512);
    ~GlyphCache();
    
    bool Initialize();
    void Shutdown();
    
    const GlyphSlot* GetGlyph(std::shared_ptr<Font> font, uint32_t codepoint);
    const GlyphSlot* GetGlyphForText(std::shared_ptr<Font> font, const std::string& text, size_t index);
    
    std::shared_ptr<Texture> GetAtlas() const { return atlas_; }
    void SetAtlas(std::shared_ptr<Texture> texture) { atlas_ = texture; }
    
    void ClearCache();
    void ClearCacheForFont(std::shared_ptr<Font> font);
    
    size_t GetCachedGlyphCount() const;
    size_t GetAtlasMemoryUsage() const;
    
    struct Statistics {
        size_t totalGlyphsCached = 0;
        size_t atlasWidth = 0;
        size_t atlasHeight = 0;
        size_t usedPixels = 0;
        float usagePercent = 0.0f;
    };
    Statistics GetStatistics() const;
    
    void SetMaxAtlasSize(uint32_t width, uint32_t height);
    bool RebuildAtlas();
    
private:
    struct CacheEntry {
        GlyphSlot slot;
        uint64_t fontKey;
        uint32_t codepoint;
    };
    
    uint64_t MakeFontKey(std::shared_ptr<Font> font) const;
    bool FindSlot(uint32_t codepoint, float& u0, float& v0, float& u1, float& v1);
    bool AllocateSlot(uint32_t width, uint32_t height, float& u0, float& v0, float& u1, float& v1);
    
    std::unordered_map<uint64_t, std::unordered_map<uint32_t, GlyphSlot>> glyphCache_;
    std::shared_ptr<Texture> atlas_;
    
    uint32_t atlasWidth_ = 512;
    uint32_t atlasHeight_ = 512;
    uint32_t currentX_ = 0;
    uint32_t currentY_ = 0;
    uint32_t rowHeight_ = 0;
    
    std::vector<uint8_t> atlasBuffer_;
    bool isInitialized_ = false;
};

} // namespace renderer
} // namespace ge
