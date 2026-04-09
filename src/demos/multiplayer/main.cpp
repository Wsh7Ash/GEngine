#include "DemoServer.h"
#include "DemoClient.h"
#include "PlayerComponent.h"
#include <iostream>
#include <string>
#include <csignal>

namespace {
    ge::demo::DemoServer* g_Server = nullptr;
    ge::demo::DemoClient* g_Client = nullptr;
}

static void signalHandler(int signal) {
    (void)signal;
    std::cout << "\nShutting down...\n";
    if (g_Server) g_Server->Shutdown();
    if (g_Client) g_Client->Shutdown();
    exit(0);
}

void printUsage(const char* programName) {
    std::cout << "GEngine Multiplayer Demo\n";
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --server              Start as server\n";
    std::cout << "  --client              Start as client\n";
    std::cout << "  --host <ip>           Server address (default: 127.0.0.1)\n";
    std::cout << "  --port <port>         Server port (default: 7777)\n";
    std::cout << "  --tick-rate <rate>    Server tick rate (default: 60)\n";
    std::cout << "  --help, -h            Show this help\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " --server\n";
    std::cout << "  " << programName << " --client --host 127.0.0.1 --port 7777\n";
}

int main(int argc, char* argv[]) {
    bool isServer = false;
    bool isClient = false;
    std::string host = "127.0.0.1";
    uint16_t port = 7777;
    int32_t tickRate = 60;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server") {
            isServer = true;
        } else if (arg == "--client") {
            isClient = true;
        } else if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::atoi(argv[++i]));
        } else if (arg == "--tick-rate" && i + 1 < argc) {
            tickRate = std::atoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    if (!isServer && !isClient) {
        std::cerr << "Error: Must specify --server or --client\n\n";
        printUsage(argv[0]);
        return 1;
    }

    if (isServer && isClient) {
        std::cerr << "Error: Cannot be both server and client\n";
        return 1;
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    if (isServer) {
        std::cout << "Starting server on port " << port << "...\n";
        ge::demo::DemoServer server;
        server.SetPort(port);
        server.SetTickRate(tickRate);
        g_Server = &server;

        server.onPlayerSpawn = [](uint32_t clientId, uint32_t playerId) {
            std::cout << "Player " << playerId << " spawned for client " << clientId << "\n";
        };

        server.Run();
    }

    if (isClient) {
        std::cout << "Connecting to " << host << ":" << port << "...\n";
        ge::demo::DemoClient client;

        client.onPlayerSpawn = [](uint32_t clientId) {
            std::cout << "Remote player " << clientId << " joined\n";
        };

        if (!client.Connect(host, port)) {
            std::cerr << "Failed to connect!\n";
            return 1;
        }

        g_Client = &client;

        std::cout << "Connected! Press Ctrl+C to disconnect.\n";
        std::cout << "Use arrow keys to move.\n";

        ge::demo::InputComponent input;

        client.Run();
    }

    return 0;
}