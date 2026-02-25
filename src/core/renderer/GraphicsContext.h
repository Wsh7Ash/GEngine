#pragma once

namespace ge {
namespace renderer {

    /**
     * @brief Interface for graphics context management.
     * Handles initialization and buffer swapping for the specific API (GL, DX11).
     */
    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;
    };

} // namespace renderer
} // namespace ge
