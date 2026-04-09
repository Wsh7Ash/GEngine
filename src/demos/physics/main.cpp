#include "PhysicsDemo.cpp"
#include <iostream>
#include <string>
#include <csignal>

namespace {
    ge::demo::PhysicsDemo* g_Demo = nullptr;
}

static void signalHandler(int signal) {
    (void)signal;
    std::cout << "\nShutting down...\n";
    if (g_Demo) g_Demo->Shutdown();
    exit(0);
}

void printUsage(const char* programName) {
    std::cout << "GEngine Physics Demo\n";
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --scene <name>        Scene to load: basic, playground, benchmark (default: basic)\n";
    std::cout << "  --speed <value>      Player move speed (default: 5.0)\n";
    std::cout << "  --no-stats           Disable FPS/stats output\n";
    std::cout << "  --help, -h           Show this help\n";
    std::cout << "\nScenes:\n";
    std::cout << "  basic         - Empty ground with player\n";
    std::cout << "  playground    - Boxes, ramps, dynamic objects\n";
    std::cout << "  benchmark     - Many dynamic objects for stress testing\n";
}

int main(int argc, char* argv[]) {
    ge::demo::PhysicsDemoConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scene" && i + 1 < argc) {
            config.scene = argv[++i];
        } else if (arg == "--speed" && i + 1 < argc) {
            config.moveSpeed = static_cast<float>(std::atof(argv[++i]));
        } else if (arg == "--no-stats") {
            config.showStats = false;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    if (config.scene != "basic" && config.scene != "playground" && config.scene != "benchmark") {
        std::cerr << "Error: Unknown scene '" << config.scene << "'\n";
        printUsage(argv[0]);
        return 1;
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    ge::demo::PhysicsDemo demo(config);
    g_Demo = &demo;

    demo.Run();

    return 0;
}