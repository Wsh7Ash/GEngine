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
using namespace ge::editor;

int main()
{
    // select renderer
    RendererAPI::SetAPI(RenderAPI::OpenGL);

    // 1. Initialize Window & Renderer
    WindowProps props("GEngine Phase 7 Refactor: Editor Module", 1280, 720);
    Window window(props);
    ge::platform::InitializeInput(&window);

    // 2. Setup ECS
    World world;
    auto renderSystem = world.RegisterSystem<RenderSystem>();
    {
        Signature signature;
        signature.set(GetComponentTypeID<TransformComponent>());
        world.SetSystemSignature<RenderSystem>(signature);
    }

    auto scriptSystem = world.RegisterSystem<ScriptSystem>();
    {
        Signature signature;
        signature.set(GetComponentTypeID<NativeScriptComponent>());
        world.SetSystemSignature<ScriptSystem>(signature);
    }

    Renderer2D::Init();
    EditorToolbar::Init(window.GetNativeWindow(), world);

    // 3. Create Camera
    auto camera2D = std::make_shared<OrthographicCamera>(-1.6f, 1.6f, -0.9f, 0.9f);
    renderSystem->Set2DCamera(camera2D);

    // --- Native Script Example ---
    class CameraController : public ScriptableEntity
    {
    public:
        void OnUpdate(float ts) override
        {
            auto& pos = GetComponent<TransformComponent>().position;
            float speed = 2.0f * ts;

            if (Input::IsKeyPressed(Key::W)) pos.y += speed;
            if (Input::IsKeyPressed(Key::S)) pos.y -= speed;
            if (Input::IsKeyPressed(Key::A)) pos.x -= speed;
            if (Input::IsKeyPressed(Key::D)) pos.x += speed;
        }
    };
    // ----------------------------

    // 4. Create Assets
    auto basicShader = Shader::Create("../src/shaders/basic.vert", "../src/shaders/basic.frag");
    auto cubeMesh = Mesh::CreateCube();

    uint32_t pixels[4 * 4] = {
        0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF, 0xFF888888,
        0xFF888888, 0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF, 0xFF888888,
        0xFF888888, 0xFFFFFFFF, 0xFF888888, 0xFFFFFFFF
    };
    auto checkerTexture = Texture::Create(4, 4, pixels, sizeof(pixels));

    // 5. Create 3D Cube
    Entity cube = world.CreateEntity();
    world.AddComponent(cube, TransformComponent{ Math::Vec3f(0.0f, 0.0f, -5.0f) });
    world.AddComponent(cube, MeshComponent{ cubeMesh, basicShader });

    // 6. Create "Sprite Forest" (10 sprites)
    for (int i = 0; i < 10; i++)
    {
        Entity sprite = world.CreateEntity();
        float x = (float)(rand() % 3200) / 1000.0f - 1.6f;
        float y = (float)(rand() % 1800) / 1000.0f - 0.9f;
        float r = (float)(rand() % 100) / 100.0f;
        float g = (float)(rand() % 100) / 100.0f;
        float b = (float)(rand() % 100) / 100.0f;

        world.AddComponent(sprite, TransformComponent{ { x, y, 0.0f }, Math::Quatf::Identity(), { 0.1f, 0.1f, 0.1f } });
        world.AddComponent(sprite, SpriteComponent{ checkerTexture, { r, g, b, 0.8f } });
    }

    // 7. Attach Camera Controller
    Entity cameraController = world.CreateEntity();
    world.AddComponent(cameraController, TransformComponent{ {0.0f, 0.0f, 0.0f} });
    world.AddComponent(cameraController, NativeScriptComponent{});
    world.GetComponent<NativeScriptComponent>(cameraController).Bind<CameraController>();

    // 8. Main Loop
    float rotation = 0.0f;
    float lastTime = 0.0f;

    while (!window.ShouldClose())
    {
        float time = (float)glfwGetTime();
        float dt = time - lastTime;
        lastTime = time;

        window.OnUpdate();

        // Update systems
        scriptSystem->Update(world, dt);

        // Rotate Cube
        rotation += 50.0f * dt;
        world.GetComponent<TransformComponent>(cube).rotation = Math::Quatf::FromEuler(Math::Vec3f(rotation, rotation * 0.5f, 0.0f));

        // Update camera position from controller
        camera2D->SetPosition(world.GetComponent<TransformComponent>(cameraController).position);

        // Rendering
        glClearColor(0.1f, 0.1f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render ECS World
        basicShader->Bind();
        Math::Mat4f projection = Math::Mat4f::Perspective(Math::DegreesToRadians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
        basicShader->SetMat4("u_ViewProjection", projection * Math::Mat4f::Identity());
        renderSystem->Render(world);

        // Render Editor UI
        ImGuiLayer::Begin();
        EditorToolbar::OnImGuiRender();
        ImGuiLayer::End();
    }

    EditorToolbar::Shutdown();
    Renderer2D::Shutdown();
    return 0;
}
