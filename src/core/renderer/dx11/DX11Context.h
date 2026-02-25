#pragma once

#include "../GraphicsContext.h"

namespace ge {
namespace renderer {

    /**
     * @brief Placeholder for Direct3D 11 Context.
     */
    class DX11Context : public GraphicsContext
    {
    public:
        DX11Context(void* windowHandle);

        virtual void Init() override;
        virtual void SwapBuffers() override;
    };

} // namespace renderer
} // namespace ge
