#include "ge_core.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>
#include <memory>

using namespace ge;
using namespace ge::ecs;
using namespace ge::platform;
using namespace ge::renderer;

int main()
{
    // Initialize logging
    ge::debug::log::Initialize();

    // Select Renderer API (OpenGL for now)
    RendererAPI::SetAPI(RenderAPI::OpenGL);
    // RendererAPI::SetAPI(RenderAPI::Dx11);
    // dx12
    // Vulkan
    // WinAPI


    // 1. Initialize Window & Renderer
    WindowProps props("GEngine Phase 4 Demo", 1280, 720);
    Window window(props);
    
    // We need to initialize the Input system with our window
    // (In a real engine, this would be handled by an Application class)
    ge::platform::InitializeInput(&window);

    // 2. Setup ECS
    World world;
    
    // Register systems
    auto renderSystem = world.RegisterSystem<RenderSystem>();
    {
        Signature signature;
        signature.set(GetComponentTypeID<TransformComponent>());
        // We'll let the system check for Mesh/Sprite internally
        world.SetSystemSignature<RenderSystem>(signature);
    }

    // 3. Create Assets
    auto basicShader = Shader::Create("../src/shaders/basic.vert", "../src/shaders/basic.frag");
    auto cubeMesh = Mesh::CreateCube();

    // Create a procedural 4x4 checkerboard texture
    uint32_t pixels[4 * 4] = {
        0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF, 0xFF888888,
        0xFF888888, 0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF, 0xFF888888,
        0xFF888888, 0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF
    };
    auto checkerTexture = Texture::Create(4, 4, pixels, sizeof(pixels));

    // 4. Create 3D Cube Entity
    Entity cube = world.CreateEntity();
    world.AddComponent(cube, TransformComponent{ Math::Vec3f(0.0f, 0.0f, -5.0f) });
    world.AddComponent(cube, MeshComponent{ cubeMesh, basicShader });

    // 5. Create 2D Sprite Entity (UI / Logo)
    Entity logo = world.CreateEntity();
    // Position it in "screenspace" or just close to camera
    // With perspective, we'll put it in front.
    world.AddComponent(logo, TransformComponent{ Math::Vec3f(1.5f, 1.0f, -4.0f), Math::Quatf::Identity(), Math::Vec3f(0.5f, 0.5f, 0.5f) });
    world.AddComponent(logo, SpriteComponent{ checkerTexture, { 1.0f, 1.0f, 1.0f, 1.0f } });
    // Use the cube mesh as a placeholder for a 2D Quad for now
    world.AddComponent(logo, MeshComponent{ cubeMesh, basicShader });

    // 6. Main Loop
    float rotation = 0.10f;
    while (!window.ShouldClose())
    {
        float dt = 1.0f / 60.0f;
        if (Input::IsKeyPressed(GLFW_KEY_ESCAPE)) break;

        // Logic: Rotate the cube
        rotation += 50.0f * dt;
        auto& t = world.GetComponent<TransformComponent>(cube);
        t.rotation = Math::Quatf::FromEuler(Math::Vec3f(rotation, rotation * 0.5f, 0.0f));

        // Rendering setup
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        basicShader->Bind();
        
        // 1. Perspective view for the whole scene (Hybrid approach)
        float fov = Math::ToRadians(60.0f);
        Math::Mat4f projection = Math::Mat4f::Perspective(fov, 1280.0f / 720.0f, 0.1f, 100.0f);
        Math::Mat4f view = Math::Mat4f::Identity(); // Camera at origin
        
        basicShader->SetMat4("u_ViewProjection", projection * view);

        // Render everything
        renderSystem->Render(world);

        window.OnUpdate();
    }

    return 0;
}
