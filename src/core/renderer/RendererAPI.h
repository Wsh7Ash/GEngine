#pragma once

namespace ge {
namespace renderer {

    enum class RenderAPI
    {
        None = 0,
        OpenGL = 1,
        DX11 = 2
    };

    /**
     * @brief Singleton-like access to the current rendering API choice.
     */
    class RendererAPI
    {
    public:
        static RenderAPI GetAPI() { return s_API; }
        static void SetAPI(RenderAPI api) { s_API = api; }

    private:
        static RenderAPI s_API;
    };

} // namespace renderer
} // namespace ge
