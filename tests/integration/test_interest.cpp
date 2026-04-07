#include "catch_amalgamated.hpp"
#include "../src/core/net/AOISpatialIndex.h"
#include "../src/core/net/InterestManager.h"
#include "../src/core/net/NetworkEntity.h"
#include <algorithm>

using namespace ge::net;
using namespace ge::ecs;

TEST_CASE("AOISpatialIndex Insert and Query", "[network][interest]")
{
    AOISpatialIndex index;
    index.cellSize = 10.0f;

    index.Insert(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 0.0f);
    index.Insert(2, Math::Vec3f(5.0f, 0.0f, 0.0f), 0.0f);
    index.Insert(3, Math::Vec3f(100.0f, 100.0f, 100.0f), 0.0f);

    auto result = index.QueryRadius(Math::Vec3f(0.0f, 0.0f, 0.0f), 20.0f);

    REQUIRE(result.size() == 2);
    REQUIRE(std::find(result.begin(), result.end(), 1) != result.end());
    REQUIRE(std::find(result.begin(), result.end(), 2) != result.end());
    REQUIRE(std::find(result.begin(), result.end(), 3) == result.end());
}

TEST_CASE("AOISpatialIndex Update and Remove", "[network][interest]")
{
    AOISpatialIndex index;
    index.cellSize = 10.0f;

    index.Insert(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 0.0f);
    index.Update(1, Math::Vec3f(50.0f, 0.0f, 0.0f));

    auto result = index.QueryRadius(Math::Vec3f(0.0f, 0.0f, 0.0f), 20.0f);
    REQUIRE(result.size() == 0);

    result = index.QueryRadius(Math::Vec3f(50.0f, 0.0f, 0.0f), 20.0f);
    REQUIRE(result.size() == 1);

    index.Remove(1);
    result = index.QueryRadius(Math::Vec3f(50.0f, 0.0f, 0.0f), 20.0f);
    REQUIRE(result.size() == 0);
}

TEST_CASE("AOISpatialIndex QueryBox", "[network][interest]")
{
    AOISpatialIndex index;
    index.cellSize = 10.0f;

    index.Insert(1, Math::Vec3f(5.0f, 5.0f, 5.0f), 0.0f);
    index.Insert(2, Math::Vec3f(15.0f, 5.0f, 5.0f), 0.0f);
    index.Insert(3, Math::Vec3f(100.0f, 100.0f, 100.0f), 0.0f);

    auto result = index.QueryBox(Math::Vec3f(0.0f, 0.0f, 0.0f), Math::Vec3f(20.0f, 10.0f, 10.0f));

    REQUIRE(result.size() == 2);
}

TEST_CASE("InterestManager Client Registration", "[network][interest]")
{
    InterestManager manager;

    manager.RegisterClient(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 50.0f, 0);
    manager.RegisterClient(2, Math::Vec3f(100.0f, 100.0f, 100.0f), 50.0f, 0);

    REQUIRE(manager.GetClientCount() == 2);
}

TEST_CASE("InterestManager Entity Registration", "[network][interest]")
{
    InterestManager manager;

    NetworkEntity netEntity;
    netEntity.viewDistance = 20.0f;
    netEntity.importance = 1.0f;
    netEntity.static_ = false;
    netEntity.replicationLayer = 0;

    manager.RegisterEntity(1, Math::Vec3f(5.0f, 0.0f, 0.0f), netEntity);

    REQUIRE(manager.GetEntityCount() == 1);
}

TEST_CASE("InterestManager Calculate Interest", "[network][interest]")
{
    InterestManager manager;

    manager.RegisterClient(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 50.0f, 0);

    NetworkEntity netEntity;
    netEntity.viewDistance = 100.0f;
    netEntity.importance = 1.0f;
    netEntity.static_ = false;
    netEntity.replicationLayer = 0;

    manager.RegisterEntity(1, Math::Vec3f(10.0f, 0.0f, 0.0f), netEntity);
    manager.RegisterEntity(2, Math::Vec3f(100.0f, 0.0f, 0.0f), netEntity);

    manager.CalculateInterest();

    auto& visible = manager.GetVisibleEntities(1);
    REQUIRE(visible.size() == 1);
    REQUIRE(visible.count(1) > 0);
    REQUIRE(visible.count(2) == 0);
}

TEST_CASE("InterestManager GetInterestedClients", "[network][interest]")
{
    InterestManager manager;

    manager.RegisterClient(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 50.0f, 0);
    manager.RegisterClient(2, Math::Vec3f(100.0f, 100.0f, 100.0f), 50.0f, 0);

    NetworkEntity netEntity;
    netEntity.viewDistance = 100.0f;
    netEntity.importance = 1.0f;
    netEntity.static_ = false;
    netEntity.replicationLayer = 0;

    manager.RegisterEntity(1, Math::Vec3f(10.0f, 0.0f, 0.0f), netEntity);

    manager.CalculateInterest();

    auto clients = manager.GetInterestedClients(1);
    REQUIRE(clients.size() == 1);
    REQUIRE(clients[0] == 1);
}

TEST_CASE("InterestManager Replication Layer Filter", "[network][interest]")
{
    InterestManager manager;

    manager.RegisterClient(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 50.0f, 1);

    NetworkEntity netEntity;
    netEntity.viewDistance = 100.0f;
    netEntity.importance = 1.0f;
    netEntity.static_ = false;
    netEntity.replicationLayer = 2;

    manager.RegisterEntity(1, Math::Vec3f(10.0f, 0.0f, 0.0f), netEntity);

    manager.CalculateInterest();

    auto& visible = manager.GetVisibleEntities(1);
    REQUIRE(visible.size() == 0);
}

TEST_CASE("InterestManager Force Add Remove", "[network][interest]")
{
    InterestManager manager;

    manager.RegisterClient(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 50.0f, 0);
    manager.ForceAddEntityToClient(1, 1);

    auto& visible = manager.GetVisibleEntities(1);
    REQUIRE(visible.size() == 1);
    REQUIRE(visible.count(1) > 0);

    manager.ForceRemoveEntityFromClient(1, 1);
    REQUIRE(visible.size() == 0);
}

TEST_CASE("InterestManager With Spatial Index", "[network][interest]")
{
    AOISpatialIndex index;
    index.cellSize = 10.0f;

    index.Insert(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 0.0f);
    index.Insert(2, Math::Vec3f(5.0f, 0.0f, 0.0f), 0.0f);
    index.Insert(3, Math::Vec3f(100.0f, 100.0f, 100.0f), 0.0f);

    InterestManager manager;
    manager.SetSpatialIndex(&index);

    manager.RegisterClient(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 20.0f, 0);

    NetworkEntity netEntity;
    netEntity.viewDistance = 100.0f;
    netEntity.importance = 1.0f;
    netEntity.static_ = false;
    netEntity.replicationLayer = 0;

    manager.RegisterEntity(1, Math::Vec3f(0.0f, 0.0f, 0.0f), netEntity);
    manager.RegisterEntity(2, Math::Vec3f(5.0f, 0.0f, 0.0f), netEntity);
    manager.RegisterEntity(3, Math::Vec3f(100.0f, 100.0f, 100.0f), netEntity);

    manager.CalculateInterest();

    auto& visible = manager.GetVisibleEntities(1);
    REQUIRE(visible.size() == 2);
    REQUIRE(visible.count(1) > 0);
    REQUIRE(visible.count(2) > 0);
    REQUIRE(visible.count(3) == 0);
}

TEST_CASE("InterestManager Update Positions", "[network][interest]")
{
    InterestManager manager;

    manager.RegisterClient(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 20.0f, 0);

    NetworkEntity netEntity;
    netEntity.viewDistance = 100.0f;
    netEntity.importance = 1.0f;
    netEntity.static_ = false;
    netEntity.replicationLayer = 0;

    manager.RegisterEntity(1, Math::Vec3f(50.0f, 0.0f, 0.0f), netEntity);

    manager.CalculateInterest();
    auto& visible1 = manager.GetVisibleEntities(1);
    REQUIRE(visible1.size() == 0);

    manager.UpdateClientPosition(1, Math::Vec3f(60.0f, 0.0f, 0.0f));
    manager.CalculateInterest();

    auto& visible2 = manager.GetVisibleEntities(1);
    REQUIRE(visible2.size() == 1);
}

TEST_CASE("InterestManager Clear", "[network][interest]")
{
    InterestManager manager;

    manager.RegisterClient(1, Math::Vec3f(0.0f, 0.0f, 0.0f), 50.0f, 0);

    NetworkEntity netEntity;
    netEntity.viewDistance = 100.0f;
    netEntity.importance = 1.0f;
    netEntity.static_ = false;
    netEntity.replicationLayer = 0;

    manager.RegisterEntity(1, Math::Vec3f(0.0f, 0.0f, 0.0f), netEntity);

    manager.Clear();

    REQUIRE(manager.GetClientCount() == 0);
    REQUIRE(manager.GetEntityCount() == 0);
}

TEST_CASE("NetworkEntity AOI Fields", "[network][interest]")
{
    NetworkEntity entity;
    entity.viewDistance = 200.0f;
    entity.importance = 2.5f;
    entity.static_ = true;
    entity.replicationLayer = 3;

    REQUIRE(entity.viewDistance == 200.0f);
    REQUIRE(entity.importance == 2.5f);
    REQUIRE(entity.static_ == true);
    REQUIRE(entity.replicationLayer == 3);
}