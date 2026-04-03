#include "ServerApplication.h"
#include "core/debug/log.h"
#include <chrono>
#include <thread>

namespace ge {

ServerApplication::ServerApplication() 
    : world_(std::make_unique<ecs::World>()) {
    net::SetGameState(net::GameState::Server);
    networkManager_ = std::make_unique<net::NetworkManager>();
    replicationManager_ = std::make_unique<net::ReplicationManager>();
}

ServerApplication::~ServerApplication() {
    Shutdown();
}

void ServerApplication::Run() {
    if (running_) return;
    
    running_ = true;
    InitializeServer();
    OnInitialize();

    debug::log::info("Server started on port {}", serverPort_);

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        accumulator_ += dt;
        
        while (accumulator_ >= tickInterval_) {
            Update(tickInterval_);
            accumulator_ -= tickInterval_;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    OnShutdown();
    debug::log::info("Server shut down");
}

void ServerApplication::Shutdown() {
    if (!running_) return;
    
    running_ = false;
    
    if (networkManager_) {
        networkManager_->CloseServer();
        networkManager_->Shutdown();
    }
}

void ServerApplication::InitializeServer() {
    if (serverInitialized_) return;

    networkManager_->Initialize(maxPlayers_);
    networkManager_->SetTickRate(tickRate_);
    
    auto server = networkManager_->CreateServer(serverPort_);
    if (!server) {
        debug::log::error("Failed to create server on port {}", serverPort_);
        return;
    }

    networkManager_->onClientConnected = [this](net::Connection* conn) {
        HandleClientConnect(conn);
    };
    
    networkManager_->onClientDisconnected = [this](net::Connection* conn) {
        HandleClientDisconnect(conn);
    };
    
    networkManager_->onMessageReceived = [this](net::Connection* conn, const net::Message& msg) {
        HandleNetworkMessage(conn, msg);
    };

    replicationManager_->Initialize(networkManager_.get());
    
    serverInitialized_ = true;
}

void ServerApplication::Update(float dt) {
    networkManager_->Update(dt);
    replicationManager_->Update(dt);
    OnUpdate(dt);
}

void ServerApplication::HandleClientConnect(net::Connection* conn) {
    debug::log::info("Client connected: {}", conn->GetId());
    OnClientConnect(conn);
    if (onClientConnected) {
        onClientConnected(conn);
    }
}

void ServerApplication::HandleClientDisconnect(net::Connection* conn) {
    debug::log::info("Client disconnected: {}", conn->GetId());
    OnClientDisconnect(conn);
    if (onClientDisconnected) {
        onClientDisconnected(conn);
    }
}

void ServerApplication::HandleNetworkMessage(net::Connection* conn, const net::Message& msg) {
    if (onMessageReceived) {
        onMessageReceived(conn, msg);
    }
}

void ServerApplication::SetMaxPlayers(int32_t max) {
    maxPlayers_ = max;
}

int32_t ServerApplication::GetMaxPlayers() const {
    return maxPlayers_;
}

void ServerApplication::SetTickRate(int32_t ticksPerSecond) {
    tickRate_ = ticksPerSecond;
    tickInterval_ = 1.0f / static_cast<float>(ticksPerSecond);
    if (networkManager_) {
        networkManager_->SetTickRate(ticksPerSecond);
    }
}

int32_t ServerApplication::GetTickRate() const {
    return tickRate_;
}

void ServerApplication::SetPort(uint16_t port) {
    serverPort_ = port;
}

uint16_t ServerApplication::GetPort() const {
    return serverPort_;
}

void ServerApplication::LoadWorld(const std::string& scenePath) {
    loadedWorldPath_ = scenePath;
    debug::log::info("Loading world from: {}", scenePath);
}

void ServerApplication::LoadWorldFromData(const std::vector<uint8_t>& sceneData) {
    worldData_ = sceneData;
    debug::log::info("Loading world from data ({} bytes)", sceneData.size());
}

} // namespace ge
