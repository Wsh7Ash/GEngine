#pragma once

#include "Window.h"
#include "../ecs/World.h"
#include <memory>

namespace ge {

struct ApplicationProps {
    std::string Name = "GEngine Application";
    uint32_t Width = 1280;
    uint32_t Height = 720;
};

namespace debug {
    class FrameBudgetProfiler;
}

class Application {
public:
    Application(const ApplicationProps& props);
    virtual ~Application();

    void Run();
    void Close();

    [[nodiscard]] platform::Window& GetWindow() { return *window_; }
    [[nodiscard]] ecs::World& GetWorld() { return *world_; }

    static Application& Get() { return *instance_; }

    debug::FrameBudgetProfiler* GetFrameProfiler() { return frameProfiler_; }

private:
    std::unique_ptr<platform::Window> window_;
    std::unique_ptr<ecs::World> world_;
    debug::FrameBudgetProfiler* frameProfiler_ = nullptr;
    bool running_ = true;

    static Application* instance_;
};

} // namespace ge