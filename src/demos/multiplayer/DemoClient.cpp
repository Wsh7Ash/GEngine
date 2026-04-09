#include "DemoClient.h"
#include "PlayerComponent.h"
#include "../core/debug/log.h"
#include <chrono>
#include <thread>
#include <cstring>

namespace ge {
namespace demo {

DemoClient::~DemoClient() {
    Disconnect();
}

bool DemoClient::Connect(const std::string& host, uint16_t port) {
    if (!networkManager_) {
        Initialize();
    }

    auto client = networkManager_->CreateClient();
    if (!client) {
        GE_LOG_ERROR("Failed to create client");
        return false;
    }

    bool connected = client->Connect(host.c_str(), port);
    if (!connected) {
        GE_LOG_ERROR("Failed to connect to {}:{}", host, port);
        return false;
    }

    connected_ = true;
    GE_LOG_INFO("Connected to {}:{}", host, port);
    return true;
}

void DemoClient::Disconnect() {
    if (networkManager_) {
        networkManager_->Shutdown();
    }
    connected_ = false;
}

void DemoClient::Run() {
    if (!connected_) {
        GE_LOG_ERROR("Not connected, call Connect() first");
        return;
    }

    running_ = true;
    GE_LOG_INFO("DemoClient running, use arrow keys to move");

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        Update(dt);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    GE_LOG_INFO("DemoClient stopped");
}

void DemoClient::Shutdown() {
    running_ = false;
}

void DemoClient::Initialize() {
    net::SetGameState(net::GameState::Client);

    networkManager_ = std::make_unique<net::NetworkManager>();
    networkManager_->Initialize(1);
    networkManager_->SetTickRate(60);

    networkManager_->onMessageReceived = [this](net::Connection*, const net::Message& msg) {
        OnNetworkMessage(msg);
    };

    initialized_ = true;
}

void DemoClient::Update(float dt) {
    if (!networkManager_) return;

    networkManager_->Update(dt);

    InterpolateRemotePlayers(dt);

    if (!pendingInput_.moveAxis.x && !pendingInput_.moveAxis.z && 
        !pendingInput_.jump && !pendingInput_.shoot) {
        return;
    }

    networkManager_->Send(0, net::MessageType::Custom, &pendingInput_, sizeof(InputComponent));
}

void DemoClient::OnNetworkMessage(const net::Message& msg) {
    if (msg.GetType() == net::MessageType::EntitySpawn) {
        const char* data = static_cast<const char*>(msg.GetData());
        uint32_t clientId = *reinterpret_cast<const uint32_t*>(data);

        RemotePlayer player;
        player.clientId = clientId;
        player.x = 0;
        player.y = 0;
        player.z = 0;
        player.interpX = 0;
        player.interpY = 0;
        player.interpZ = 0;

        remotePlayers_.push_back(player);
        remotePlayerIndices_[clientId] = remotePlayers_.size() - 1;

        GE_LOG_INFO("Remote player spawned: {}", clientId);

        if (onPlayerSpawn) {
            onPlayerSpawn(clientId);
        }
    }
    else if (msg.GetType() == net::MessageType::EntityUpdate) {
        const char* data = static_cast<const char*>(msg.GetData());
        uint32_t clientId = *reinterpret_cast<const uint32_t*>(data);
        const float* pos = reinterpret_cast<const float*>(data + 4);

        auto it = remotePlayerIndices_.find(clientId);
        if (it != remotePlayerIndices_.end()) {
            remotePlayers_[it->second].x = pos[0];
            remotePlayers_[it->second].y = pos[1];
            remotePlayers_[it->second].z = pos[2];
        }
    }
    else if (msg.GetType() == net::MessageType::EntityDestroy) {
        const char* data = static_cast<const char*>(msg.GetData());
        uint32_t clientId = *reinterpret_cast<const uint32_t*>(data);

        auto it = remotePlayerIndices_.find(clientId);
        if (it != remotePlayerIndices_.end()) {
            remotePlayers_.erase(remotePlayers_.begin() + it->second);
            remotePlayerIndices_.erase(it);
            GE_LOG_INFO("Remote player destroyed: {}", clientId);
        }
    }
}

void DemoClient::InterpolateRemotePlayers(float dt) {
    float interpSpeed = 10.0f;

    for (auto& player : remotePlayers_) {
        float dx = player.x - player.interpX;
        float dz = player.z - player.interpZ;
        float dist = dx * dx + dz * dz;

        if (dist > 0.001f) {
            player.interpX += dx * interpSpeed * dt;
            player.interpZ += dz * interpSpeed * dt;
        } else {
            player.interpX = player.x;
            player.interpZ = player.z;
        }
    }
}

void DemoClient::SetInput(const InputComponent& input) {
    pendingInput_ = input;
}

void DemoClient::UpdateLocalPosition(float x, float z) {
    localX_ = x;
    localZ_ = z;
}

} // namespace demo
} // namespace ge