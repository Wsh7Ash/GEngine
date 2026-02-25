#include "ge_core.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>

using namespace ge;
using namespace ge::ecs;
using namespace ge::platform;
using namespace ge::renderer;

int main()
{
    // Initialize logging
    ge::debug::log::Initialize();

    // 1. Initialize Window & Renderer
    WindowProps props("GEngine Phase 3 Demo", 1280, 720);
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
        signature.set(GetComponentTypeID<MeshComponent>());
        world.SetSystemSignature<RenderSystem>(signature);
    }

    // 3. Create Assets
    Shader basicShader("../src/shaders/basic.vert", "../src/shaders/basic.frag");
    Mesh* cubeMesh = Mesh::CreateCube();

    // 4. Create Entity
    Entity cube = world.CreateEntity();
    world.AddComponent(cube, TransformComponent{ Math::Vec3f(0.0f, 0.0f, -5.0f) });
    world.AddComponent(cube, MeshComponent{ cubeMesh, &basicShader });

    // 5. Main Loop
    float rotation = 0.0f;
    while (!window.ShouldClose())
    {
        // Delta time (hardcoded for now, normally from a clock)
        float dt = 1.0f / 60.0f;

        // Input handling
        if (Input::IsKeyPressed(GLFW_KEY_ESCAPE))
            break;

        // Logic: Rotate the cube
        rotation += 50.0f * dt;
        auto& t = world.GetComponent<TransformComponent>(cube);
        t.rotation = Math::Quatf::FromEuler(Math::Vec3f(rotation, rotation * 0.5f, 0.0f));

        // Interaction: Move cube with WASD
        float speed = 5.0f * dt;
        if (Input::IsKeyPressed(GLFW_KEY_W)) t.position.y += speed;
        if (Input::IsKeyPressed(GLFW_KEY_S)) t.position.y -= speed;
        if (Input::IsKeyPressed(GLFW_KEY_A)) t.position.x -= speed;
        if (Input::IsKeyPressed(GLFW_KEY_D)) t.position.x += speed;

        // Rendering
        // (In a real engine, we'd have a Renderer::BeginFrame / EndFrame)
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Global Camera Uniforms
        basicShader.Bind();
        
        Math::Mat4f view = Math::Mat4f::Identity(); // View matrix (cam at origin)
        // Camera usually looks down -Z. Our cube is at -5.0 on Z.
        
        float fov = 60.0f * (3.14159265f / 180.0f);
        Math::Mat4f projection = Math::Mat4f::Perspective(fov, 1280.0f / 720.0f, 0.1f, 100.0f);
        
        basicShader.SetMat4("u_View", view);
        basicShader.SetMat4("u_Projection", projection);

        renderSystem->Render(world);

        window.OnUpdate();
    }

    delete cubeMesh;

    return 0;
}
