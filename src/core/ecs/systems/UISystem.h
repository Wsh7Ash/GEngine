#pragma once

#include "../../ge_core.h"
#include "../System.h"

namespace ge {
namespace ecs {

    /**
     * @brief System responsible for UI layout and rendering.
     */
    class UISystem : public System
    {
    public:
        UISystem();
        
        void Update(World& world, float dt, const Math::Vec2f& viewportSize);
        void Render(World& world, const Math::Vec2f& viewportSize);

    private:
        void UpdateRectTransform(RectTransformComponent& rect, const Math::Vec2f& screenRes);
        
        std::shared_ptr<renderer::OrthographicCamera> ui_camera_;
    };

} // namespace ecs
} // namespace ge
