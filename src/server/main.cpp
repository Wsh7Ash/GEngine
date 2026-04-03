#include "src/server/ServerApplication.h"
#include "src/core/debug/log.h"
#include <iostream>
#include <csignal>

namespace ge {
    static ServerApplication* g_Server = nullptr;
}

static void signalHandler(int signal) {
    (void)signal;
    if (ge::g_Server) {
        ge::g_Server->Shutdown();
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    ge::debug::log::Initialize();

    ge::ApplicationProps props;
    props.Name = "GEngine Dedicated Server";

    ge::ServerApplication server;
    ge::g_Server = &server;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    uint16_t port = 7777;
    int32_t maxPlayers = 32;
    int32_t tickRate = 60;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::atoi(argv[++i]));
        } else if (arg == "--max-players" && i + 1 < argc) {
            maxPlayers = std::atoi(argv[++i]);
        } else if (arg == "--tick-rate" && i + 1 < argc) {
            tickRate = std::atoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "GEngine Dedicated Server\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --port <port>       Server port (default: 7777)\n";
            std::cout << "  --max-players <n>   Maximum players (default: 32)\n";
            std::cout << "  --tick-rate <rate>  Tick rate (default: 60)\n";
            std::cout << "  --help, -h          Show this help\n";
            return 0;
        }
    }

    server.SetPort(port);
    server.SetMaxPlayers(maxPlayers);
    server.SetTickRate(tickRate);

    ge::debug::log::info("Starting GEngine Dedicated Server");
    ge::debug::log::info("Port: {}, MaxPlayers: {}, TickRate: {}", port, maxPlayers, tickRate);

    server.Run();

    return 0;
}
