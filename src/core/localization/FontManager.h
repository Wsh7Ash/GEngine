#pragma once

// ================================================================
//  FontManager.h
//  Font loading and management for localization system.
// ================================================================

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstdint>

namespace ge {
namespace renderer {

struct GlyphMetrics {
    uint32_t codepoint;
    float advanceX = 0.0f;
    float advanceY = 0.0f;
    float bitmapLeft = 0.0f;
    float bitmapTop = 0.0f;
    uint32_t bitmapWidth = 0;
    uint32_t bitmapHeight = 0;
    float u0 = 0.0f, v0 = 0.0f;
    float u1 = 0.0f, v1 = 0.0f;
};

struct FontInfo {
    std::string family;
    std::string style;
    float pointSize = 0.0f;
    float lineHeight = 0.0f;
    float ascender = 0.0f;
    float descender = 0.0f;
};

struct FontMetrics {
    float unitsPerEm = 0.0f;
    float ascender = 0.0f;
    float descender = 0.0f;
    float lineGap = 0.0f;
    int numGlyphs = 0;
};

class Font {
public:
    virtual ~Font() = default;
    
    virtual bool LoadFromFile(const std::string& filepath, float pointSize) = 0;
    virtual bool LoadFromMemory(const uint8_t* data, size_t size, float pointSize) = 0;
    
    virtual const GlyphMetrics* GetGlyph(uint32_t codepoint) const = 0;
    virtual float GetKerning(uint32_t leftCodepoint, uint32_t rightCodepoint) const = 0;
    virtual const FontMetrics& GetMetrics() const = 0;
    virtual const FontInfo& GetInfo() const = 0;
    
    virtual float GetStringWidth(const std::string& text, float scale = 1.0f) const = 0;
    virtual float GetStringHeight(const std::string& text, float scale = 1.0f) const = 0;
    
    virtual std::vector<uint32_t> GetSupportedCodepoints() const = 0;
    virtual bool HasGlyph(uint32_t codepoint) const = 0;
    
    virtual float GetPointSize() const = 0;
    virtual void SetPointSize(float size) = 0;
};

class FontManager {
public:
    static FontManager& Get();
    
    FontManager();
    ~FontManager();
    
    void Initialize();
    void Shutdown();
    
    std::shared_ptr<Font> LoadFont(const std::string& name, const std::string& filepath, float pointSize);
    std::shared_ptr<Font> GetFont(const std::string& name);
    void UnloadFont(const std::string& name);
    void UnloadAllFonts();
    
    bool HasFont(const std::string& name) const;
    std::vector<std::string> GetLoadedFontNames() const;
    
    void SetDefaultFont(const std::string& name);
    std::shared_ptr<Font> GetDefaultFont();
    
private:
    struct FontEntry {
        std::shared_ptr<Font> font;
        std::string filepath;
        float pointSize;
    };
    
    std::unordered_map<std::string, FontEntry> loadedFonts_;
    std::string defaultFontName_;
    bool isInitialized_ = false;
};

std::shared_ptr<Font> CreateFont(const std::string& type = "freetype");

} // namespace renderer
} // namespace ge
