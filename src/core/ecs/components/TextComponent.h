#pragma once

// ================================================================
//  TextComponent.h
//  ECS component for localized text rendering.
// ================================================================

#include "../../localization/TextRenderer.h"
#include "../../localization/LocalizationManager.h"
#include "RectTransformComponent.h"
#include <string>
#include <memory>

namespace ge {
namespace ecs {

struct TextComponent {
    std::string text;
    std::string localizationKey;
    std::string fontName;
    float fontSize = 16.0f;
    Math::Vec4f color = {1.0f, 1.0f, 1.0f, 1.0f};
    float outlineWidth = 0.0f;
    Math::Vec4f outlineColor = {0.0f, 0.0f, 0.0f, 1.0f};
    bool dropShadow = false;
    Math::Vec2f shadowOffset = {1.0f, 1.0f};
    Math::Vec4f shadowColor = {0.0f, 0.0f, 0.0f, 0.5f};
    
    renderer::TextAlignment alignment = renderer::TextAlignment::Left;
    renderer::TextOverflow overflow = renderer::TextOverflow::Wrap;
    float lineSpacing = 1.0f;
    float letterSpacing = 0.0f;
    
    float maxWidth = 0.0f;
    bool autoSize = true;
    bool localize = true;
    
    bool dirty = true;
    
    renderer::TextLayout cachedLayout;
    
    void SetText(const std::string& newText) {
        if (text != newText) {
            text = newText;
            localizationKey.clear();
            dirty = true;
        }
    }
    
    void SetLocalizedText(const std::string& key) {
        if (localizationKey != key) {
            localizationKey = key;
            dirty = true;
        }
    }
    
    void UpdateLocalization(const localization::LocalizationManager& loc) {
        if (localize && !localizationKey.empty()) {
            text = loc.Get(localizationKey);
            dirty = true;
        }
    }
    
    renderer::TextStyle GetTextStyle() const {
        renderer::TextStyle style;
        style.fontSize = fontSize;
        style.color = color;
        style.outlineWidth = outlineWidth;
        style.outlineColor = outlineColor;
        style.dropShadow = dropShadow;
        style.shadowOffsetX = shadowOffset.x;
        style.shadowOffsetY = shadowOffset.y;
        style.shadowColor = shadowColor;
        style.alignment = alignment;
        style.overflow = overflow;
        style.lineSpacing = lineSpacing;
        style.letterSpacing = letterSpacing;
        return style;
    }
};

} // namespace ecs
} // namespace ge
