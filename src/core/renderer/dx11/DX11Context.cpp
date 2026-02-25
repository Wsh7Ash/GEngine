#include "DX11Context.h"
#include "../../debug/log.h"

namespace ge {
namespace renderer {

    DX11Context::DX11Context(void* windowHandle)
    {
        (void)windowHandle;
    }

    void DX11Context::Init()
    {
        GE_LOG_INFO("Initializing DX11 Context (Placeholder)");
    }

    void DX11Context::SwapBuffers()
    {
        // DX11 SwapChain->Present
    }

} // namespace renderer
} // namespace ge
