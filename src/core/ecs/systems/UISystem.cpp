#include "UISystem.h"
#include "../../renderer/Renderer2D.h"
#include "../../renderer/Texture.h"
#include "../../assets/AssetManager.h"
#include "../../assets/TextureAsset.h"
#include "../../platform/Input.h"

namespace ge {
namespace ecs {

    UISystem::UISystem()
    {
    }

    void UISystem::Update(World& world, float dt, const Math::Vec2f& viewportSize)
    {
        auto [mx, my] = platform::Input::GetMousePosition();
        Math::Vec2f mousePos = { mx, my };
        // Mouse position from GLFW is usually Top-Left (0,0).
        // Our UI camera is Bottom-Left (0,0) to Top-Right (W,H). (?)
        // Wait, OrthographicCamera(0, W, 0, H) is Bottom-Left 0,0.
        // GLFW is Top-Left 0,0. Let's flip mouse Y.
        mousePos.y = viewportSize.y - mousePos.y;

        for (auto entity : world.Query<RectTransformComponent>())
        {
            auto& rect = world.GetComponent<RectTransformComponent>(entity);
            UpdateRectTransform(rect, viewportSize);
            
            if (world.HasComponent<UIButtonComponent>(entity))
            {
                auto& button = world.GetComponent<UIButtonComponent>(entity);
                
                // Hit test
                bool isInside = (mousePos.x >= rect.ScreenPosition.x - rect.ScreenSize.x * 0.5f &&
                                 mousePos.x <= rect.ScreenPosition.x + rect.ScreenSize.x * 0.5f &&
                                 mousePos.y >= rect.ScreenPosition.y - rect.ScreenSize.y * 0.5f &&
                                 mousePos.y <= rect.ScreenPosition.y + rect.ScreenSize.y * 0.5f);

                button.IsMouseOver = isInside;
                if (isInside)
                {
                    if (platform::Input::IsMouseButtonPressed(0)) // Left Click
                        button.State = ButtonState::Pressed;
                    else
                        button.State = ButtonState::Hovered;
                }
                else
                {
                    button.State = ButtonState::Normal;
                }
            }
        }
    }

    void UISystem::Render(World& world, const Math::Vec2f& viewportSize)
    {
        // Setup UI Camera for current viewport (Bottom-Left 0,0)
        renderer::OrthographicCamera uiCamera(0.0f, viewportSize.x, 0.0f, viewportSize.y);
        renderer::Renderer2D::BeginScene(uiCamera);

        for (auto entity : world.Query<RectTransformComponent, UIImageComponent>())
        {
            auto& rect = world.GetComponent<RectTransformComponent>(entity);
            auto& image = world.GetComponent<UIImageComponent>(entity);

            Math::Vec4f color = image.Color;
            
            if (world.HasComponent<UIButtonComponent>(entity))
            {
                auto& button = world.GetComponent<UIButtonComponent>(entity);
                switch (button.State)
                {
                    case ButtonState::Hovered: color = button.HoverColor; break;
                    case ButtonState::Pressed: color = button.PressedColor; break;
                    case ButtonState::Disabled: color = button.DisabledColor; break;
                    default: color = button.NormalColor; break;
                }
            }

            if (image.TextureHandle != 0)
            {
                auto textureAsset = assets::AssetManager::GetAsset<assets::TextureAsset>(image.TextureHandle);
                if (textureAsset && textureAsset->Texture)
                {
                    renderer::Renderer2D::DrawQuad(rect.ScreenPosition, rect.ScreenSize, textureAsset->Texture, color);
                }
                else
                {
                    renderer::Renderer2D::DrawQuad(rect.ScreenPosition, rect.ScreenSize, color);
                }
            }
            else
            {
                renderer::Renderer2D::DrawQuad(rect.ScreenPosition, rect.ScreenSize, color);
            }
        }

        renderer::Renderer2D::EndScene();
    }

    void UISystem::UpdateRectTransform(RectTransformComponent& rect, const Math::Vec2f& viewportSize)
    {
        Math::Vec2f anchorMinPos = { 
            Math::Lerp(0.0f, viewportSize.x, rect.AnchorMin.x),
            Math::Lerp(0.0f, viewportSize.y, rect.AnchorMin.y)
        };
        Math::Vec2f anchorMaxPos = {
            Math::Lerp(0.0f, viewportSize.x, rect.AnchorMax.x),
            Math::Lerp(0.0f, viewportSize.y, rect.AnchorMax.y)
        };

        rect.ScreenSize = rect.Size;
        if (rect.AnchorMin.x != rect.AnchorMax.x)
            rect.ScreenSize.x = (anchorMaxPos.x - anchorMinPos.x) + rect.Size.x;
        if (rect.AnchorMin.y != rect.AnchorMax.y)
            rect.ScreenSize.y = (anchorMaxPos.y - anchorMinPos.y) + rect.Size.y;

        Math::Vec2f anchorCenter = {
            Math::Lerp(anchorMinPos.x, anchorMaxPos.x, 0.5f),
            Math::Lerp(anchorMinPos.y, anchorMaxPos.y, 0.5f)
        };

        // DrawQuad expects CENTER position.
        // Pivot (0.5, 0.5) means position is at center.
        rect.ScreenPosition = anchorCenter + rect.Position - (rect.ScreenSize * (rect.Pivot - Math::Vec2f(0.5f)));
    }

} // namespace ecs
} // namespace ge
