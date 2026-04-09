#include "Application.h"
#include <memory>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "ImGuiLayer.h"
#include "../renderer/Renderer2D.h"
#include "../renderer/ShaderVariantManager.h"
#include "../debug/log.h"
#include "../debug/FrameBudgetProfiler.h"
#include "VFS.h"

#include "../ecs/World.h"
#include "../ecs/SystemManager.h"
#include "../ecs/ComponentRegistry.h"
#include "../ecs/systems/Physics2DSystem.h"
#include "../ecs/systems/Physics3DSystem.h"
#include "../ecs/systems/UISystem.h"
#include "../ecs/systems/ScriptSystem.h"
#include "../ecs/systems/AudioSystem.h"
#include "../ecs/systems/AudioSystem.h"
#include "../ecs/systems/ParticleSystem.h"
#include "../ecs/systems/AnimationSystem.h"
#include "../ecs/systems/PostProcessSystem.h"
#include "../ecs/systems/RenderSystem.h"

#include "../ecs/components/Rigidbody2DComponent.h"
#include "../ecs/components/Rigidbody3DComponent.h"
#include "../ecs/components/RectTransformComponent.h"
#include "../ecs/components/AudioSourceComponent.h"
#include "../ecs/components/ParticleEmitterComponent.h"
#include "../ecs/components/AnimatorComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/TransformComponent.h"

#ifndef GE_STANDALONE
#include "../editor/EditorToolbar.h"
#endif

namespace ge {

Application* Application::instance_ = nullptr;

Application::Application(const ApplicationProps& props) {
    if (instance_) {
        GE_LOG_ERROR("Application already exists!");
        return;
    }
    instance_ = this;

    ge::renderer::RendererAPI::SetAPI(ge::renderer::RenderAPI::OpenGL);
    core::VFS::Init();
    
    window_ = std::make_unique<ge::platform::Window>(ge::platform::WindowProps(props.Name, props.Width, props.Height));
    world_ = std::make_unique<ge::ecs::World>();
    
    frameProfiler_ = new ge::debug::FrameBudgetProfiler();
    frameProfiler_->Initialize();
    
    std::bitset<128> signature;

    // Register Core Systems
    auto physicsSystem = world_->RegisterSystem<ecs::Physics2DSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::Rigidbody2DComponent>());
        signature.set(ecs::GetComponentTypeID<ecs::TransformComponent>());
        world_->SetSystemSignature<ecs::Physics2DSystem>(signature);
    }

    auto physics3DSystem = world_->RegisterSystem<ecs::Physics3DSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::Rigidbody3DComponent>());
        signature.set(ecs::GetComponentTypeID<ecs::TransformComponent>());
        world_->SetSystemSignature<ecs::Physics3DSystem>(signature);
    }

    auto uiSystem = world_->RegisterSystem<ecs::UISystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::RectTransformComponent>());
        world_->SetSystemSignature<ecs::UISystem>(signature);
    }

    auto audioSystem = world_->RegisterSystem<ecs::AudioSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::AudioSourceComponent>());
        world_->SetSystemSignature<ecs::AudioSystem>(signature);
    }

    auto particleSystem = world_->RegisterSystem<ecs::ParticleSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::ParticleEmitterComponent>());
        world_->SetSystemSignature<ecs::ParticleSystem>(signature);
    }

    auto animationSystem = world_->RegisterSystem<ecs::AnimationSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::AnimatorComponent>());
        signature.set(ecs::GetComponentTypeID<ecs::SpriteComponent>());
        world_->SetSystemSignature<ecs::AnimationSystem>(signature);
    }

    auto renderSystem = world_->RegisterSystem<ecs::RenderSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::TransformComponent>());
        world_->SetSystemSignature<ecs::RenderSystem>(signature);
    }

    auto postProcessSystem = world_->RegisterSystem<ecs::PostProcessSystem>();

    auto scriptSystem = world_->RegisterSystem<ecs::ScriptSystem>();
    {
        signature.reset();
        signature.set(ecs::GetComponentTypeID<ecs::NativeScriptComponent>());
        signature.set(ecs::GetComponentTypeID<scripting::ManagedScriptComponent>());
        signature.set(ecs::GetComponentTypeID<ecs::TransformComponent>());
        world_->SetSystemSignature<ecs::ScriptSystem>(signature);
    }

    renderer::Renderer2D::Init();
    renderer::ShaderWarmUp::Get().WarmUp();
    audioSystem->Init();
    postProcessSystem->Init(props.Width, props.Height);
    
#ifndef GE_STANDALONE
    editor::EditorToolbar::Init((GLFWwindow*)window_->GetNativeWindow(), *world_);
#endif
}

Application::~Application() {
    if (frameProfiler_) {
        frameProfiler_->Shutdown();
        delete frameProfiler_;
        frameProfiler_ = nullptr;
    }
    renderer::ShaderVariantManager::Get().StopBackgroundThread();
    renderer::Renderer2D::Shutdown();
#ifndef GE_STANDALONE
    editor::EditorToolbar::Shutdown();
#endif
}

void Application::Run() {
    float lastTime = 0.0f;

    while (running_ && !window_->ShouldClose()) {
        float time = (float)glfwGetTime();
        float dt = (lastTime == 0.0f) ? 0.0f : time - lastTime;
        lastTime = time;

        window_->OnUpdate();

        if (frameProfiler_) {
            frameProfiler_->BeginFrame();
        }
#ifndef GE_STANDALONE
        if (editor::EditorToolbar::GetState() == editor::SceneState::Play) {
#endif
            world_->GetSystem<ecs::Physics2DSystem>()->Update(*world_, dt);
            world_->GetSystem<ecs::Physics3DSystem>()->Update(*world_, dt);
            world_->GetSystem<ecs::UISystem>()->Update(*world_, dt, { (float)window_->GetWidth(), (float)window_->GetHeight() });
            world_->GetSystem<ecs::AudioSystem>()->Update(*world_, dt);
            world_->GetSystem<ecs::ParticleSystem>()->Update(*world_, dt);
            world_->GetSystem<ecs::AnimationSystem>()->Update(*world_, dt);
#ifndef GE_STANDALONE
        }
#endif

        // 2. Render Pass
#ifndef GE_STANDALONE
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGuiLayer::Begin();
        editor::EditorToolbar::OnImGuiRender();
        ImGuiLayer::End();
#else
        // Standalone rendering logic
        auto renderSystem = world_->GetSystem<ecs::RenderSystem>();
        renderSystem->Render(*world_, dt);
#endif
    }
}

void Application::Close() {
    running_ = false;
}

} // namespace ge
