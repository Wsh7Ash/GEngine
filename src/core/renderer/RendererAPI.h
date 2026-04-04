#pragma once

namespace ge {
namespace renderer {

    enum class RenderAPI
    {
        None = 0,
        OpenGL = 1,
        DX11 = 2,
        Vulkan = 3,
        WebGL2 = 4
    };

    /**
     * @brief Singleton-like access to the current rendering API choice.
     * 
     * @note API selection is COMPILE-TIME / STARTUP-ONLY
     * 
     * The RenderAPI must be set BEFORE creating any graphics resources
     * (Window, Shader, Mesh, Framebuffer, etc.). After that point, changing
     * the API will result in undefined behavior.
     * 
     * Usage:
     * - Set API in Application constructor before window creation
     * - OR set via command-line argument parsed before Application
     * - Factory methods (Shader::Create, Mesh::Create, etc.) dispatch
     *   based on this value to create the appropriate backend
     * 
     * Example:
     * @code
     *   // At startup, before creating any graphics objects:
     *   RendererAPI::SetAPI(RenderAPI::Vulkan);
     *   
     *   // Then create window and graphics resources:
     *   auto window = std::make_unique<Window>(...);  // Uses Vulkan
     *   auto shader = Shader::Create(...);           // Creates VulkanShader
     * @endcode
     */
    class RendererAPI
    {
    public:
        /**
         * @brief Get the current rendering API.
         * @return The currently selected RenderAPI
         */
        static RenderAPI GetAPI() { return s_API; }
        
        /**
         * @brief Set the rendering API.
         * 
         * @warning MUST be called BEFORE any graphics resources are created.
         * @warning After Window creation, this should NOT be changed.
         * 
         * @param api The RenderAPI to use
         */
        static void SetAPI(RenderAPI api) { s_API = api; }

    private:
        static RenderAPI s_API;
    };

} // namespace renderer
} // namespace ge
