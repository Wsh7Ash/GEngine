#include "src/core/platform/Application.h"
#include <iostream>

using namespace ge;

class GEngineApp : public Application {
public:
    GEngineApp(const ApplicationProps& props) : Application(props) {
        // Here you would setup your scene, add entities, etc.
        // For Phase 40, we'll just let the base class handle initialization.
    }
    
    ~GEngineApp() override = default;
};

int main() {
    ge::debug::log::Initialize();
    
    ApplicationProps props;
    props.Name = "GEngine Standalone - Phase 40";
    props.Width = 1280;
    props.Height = 720;
    
    auto app = std::make_unique<GEngineApp>(props);
    app->Run();
    
    return 0;
}
