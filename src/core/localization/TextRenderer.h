#pragma once

// ================================================================
//  TextRenderer.h
//  UTF-8 text rendering with localization support.
// ================================================================

#include "FontManager.h"
#include "GlyphCache.h"
#include "../math/VecTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace ge {
namespace renderer {

enum class TextAlignment {
    Left,
    Center,
    Right,
    Justified
};

enum class TextOverflow {
    Wrap,
    Ellipsis,
    Clip
};

struct TextStyle {
    std::shared_ptr<Font> font = nullptr;
    float fontSize = 16.0f;
    Math::Vec4f color = {1.0f, 1.0f, 1.0f, 1.0f};
    float outlineWidth = 0.0f;
    Math::Vec4f outlineColor = {0.0f, 0.0f, 0.0f, 1.0f};
    float shadowOffsetX = 0.0f;
    float shadowOffsetY = 0.0f;
    Math::Vec4f shadowColor = {0.0f, 0.0f, 0.0f, 0.5f};
    bool dropShadow = false;
    TextAlignment alignment = TextAlignment::Left;
    TextOverflow overflow = TextOverflow::Wrap;
    float lineSpacing = 1.0f;
    float letterSpacing = 0.0f;
};

struct RenderedGlyph {
    uint32_t codepoint;
    Math::Vec2f position;
    Math::Vec2f size;
    Math::Vec4f uv;
    Math::Vec4f color;
};

struct TextLayout {
    std::vector<RenderedGlyph> glyphs;
    float width = 0.0f;
    float height = 0.0f;
    int lineCount = 1;
    std::vector<float> lineWidths;
};

class TextRenderer {
public:
    static TextRenderer& Get();
    
    TextRenderer();
    ~TextRenderer();
    
    void Initialize();
    void Shutdown();
    
    TextLayout CalculateLayout(const std::string& text, const TextStyle& style, float maxWidth = 0.0f);
    TextLayout CalculateLayout(const std::wstring& text, const TextStyle& style, float maxWidth = 0.0f);
    
    void Render(const std::string& text, const TextStyle& style, const Math::Vec2f& position);
    void Render(const std::wstring& text, const TextStyle& style, const Math::Vec2f& position);
    
    std::vector<uint32_t> UTF8ToCodepoints(const std::string& utf8) const;
    std::string CodepointsToUTF8(const std::vector<uint32_t>& codepoints) const;
    std::wstring UTF8ToWideString(const std::string& utf8) const;
    std::string WideStringToUTF8(const std::wstring& wide) const;
    
    void SetGlyphCache(std::shared_ptr<GlyphCache> cache) { glyphCache_ = cache; }
    std::shared_ptr<GlyphCache> GetGlyphCache() const { return glyphCache_; }
    
    void SetDefaultStyle(const TextStyle& style) { defaultStyle_ = style; }
    const TextStyle& GetDefaultStyle() const { return defaultStyle_; }
    
private:
    struct UTF8Iterator {
        const std::string* str = nullptr;
        size_t pos = 0;
        
        UTF8Iterator() = default;
        explicit UTF8Iterator(const std::string& s) : str(&s), pos(0) {}
        
        bool HasNext() const { return str && pos < str->size(); }
        
        uint32_t Next() {
            if (!HasNext()) return 0;
            
            uint8_t first = (*str)[pos];
            if ((first & 0x80) == 0) {
                pos++;
                return first;
            }
            if ((first & 0xE0) == 0xC0) {
                if (pos + 1 >= str->size()) return 0;
                uint32_t codepoint = (first & 0x1F) << 6;
                codepoint |= ((*str)[pos + 1] & 0x3F);
                pos += 2;
                return codepoint;
            }
            if ((first & 0xF0) == 0xE0) {
                if (pos + 2 >= str->size()) return 0;
                uint32_t codepoint = (first & 0x0F) << 12;
                codepoint |= ((*str)[pos + 1] & 0x3F) << 6;
                codepoint |= ((*str)[pos + 2] & 0x3F);
                pos += 3;
                return codepoint;
            }
            if ((first & 0xF8) == 0xF0) {
                if (pos + 3 >= str->size()) return 0;
                uint32_t codepoint = (first & 0x07) << 18;
                codepoint |= ((*str)[pos + 1] & 0x3F) << 12;
                codepoint |= ((*str)[pos + 2] & 0x3F) << 6;
                codepoint |= ((*str)[pos + 3] & 0x3F);
                pos += 4;
                return codepoint;
            }
            pos++;
            return 0;
        }
    };
    
    TextLayout LayoutLine(const std::vector<uint32_t>& codepoints, const TextStyle& style, size_t start, size_t end, float yOffset);
    float CalculateLineWidth(const std::vector<uint32_t>& codepoints, const TextStyle& style, size_t start, size_t end);
    
    std::shared_ptr<GlyphCache> glyphCache_;
    TextStyle defaultStyle_;
    bool isInitialized_ = false;
};

} // namespace renderer
} // namespace ge
