#include "catch_amalgamated.hpp"
#include "../src/core/net/Message.h"
#include "../src/core/net/GameState.h"
#include "../src/core/net/NetworkMode.h"
#include "../src/core/net/ReplicationAttributes.h"

TEST_CASE("Message Type Enum", "[network]")
{
    REQUIRE(static_cast<int>(ge::net::MessageType::Ping) >= 0);
    REQUIRE(static_cast<int>(ge::net::MessageType::Pong) >= 0);
    REQUIRE(static_cast<int>(ge::net::MessageType::Connect) >= 0);
    REQUIRE(static_cast<int>(ge::net::MessageType::Disconnect) >= 0);
}

TEST_CASE("Message Header Size", "[network]")
{
    REQUIRE(sizeof(ge::net::MessageHeader) >= sizeof(uint32_t));
}

TEST_CASE("GameState IsServer", "[network]")
{
    REQUIRE(ge::net::GameState::Server == ge::net::GameState::Server);
    REQUIRE(ge::net::GameState::Client == ge::net::GameState::Client);
    REQUIRE(ge::net::GameState::Server != ge::net::GameState::Client);
}

TEST_CASE("NetworkMode Singleton", "[network]")
{
    auto& mode = ge::net::NetworkMode::Get();
    REQUIRE(mode.GetState() == ge::net::GameState::Client);
}

TEST_CASE("Replication Attribute Macro", "[network]")
{
    struct TestComponent {
        GE_REPLICATED(float, position);
        GE_REPLICATED(int, health);
    };
    
    TestComponent comp;
    comp.position = 10.0f;
    comp.health = 100;
    
    REQUIRE(comp.position == 10.0f);
    REQUIRE(comp.health == 100);
}
