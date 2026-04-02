#pragma once

#include "NavMeshTypes.h"
#include "../ecs/World.h"
#include "../ecs/Entity.h"
#include <memory>
#include <vector>

namespace ge {
namespace navigation {

struct MeshTriangle {
    Math::Vec3f vertices[3];
    Math::Vec3f normal;
    
    MeshTriangle() = default;
    MeshTriangle(const Math::Vec3f& v0, const Math::Vec3f& v1, const Math::Vec3f& v2) {
        vertices[0] = v0;
        vertices[1] = v1;
        vertices[2] = v2;
        Math::Vec3f edge1 = v1 - v0;
        Math::Vec3f edge2 = v2 - v0;
        normal = edge1.Cross(edge2).Normalized();
    }
};

struct NavMeshBuildData {
    std::vector<float> vertices;
    std::vector<int> triangles;
    Math::Vec3f bMin;
    Math::Vec3f bMax;
    
    void Clear() {
        vertices.clear();
        triangles.clear();
    }
    
    bool IsEmpty() const { return vertices.empty(); }
};

class NavMeshGenerator {
public:
    NavMeshGenerator();
    explicit NavMeshGenerator(const NavMeshConfig& config);
    ~NavMeshGenerator();
    
    void SetConfig(const NavMeshConfig& config);
    const NavMeshConfig& GetConfig() const { return config_; }
    
    void SetBoundingBox(const Math::Vec3f& min, const Math::Vec3f& max);
    void ComputeBoundingBox(const std::vector<MeshTriangle>& triangles);
    
    bool Build(NavMesh& navMesh);
    bool BuildTile(NavMesh& navMesh, int tileX, int tileZ);
    
    void AddTriangleMesh(const std::vector<MeshTriangle>& triangles);
    void AddBox(const Math::Vec3f& min, const Math::Vec3f& max);
    void AddBox(const Math::Vec3f& center, const Math::Vec3f& halfExtents, const Math::Quatf& rotation);
    void AddCylinder(const Math::Vec3f& center, float radius, float height);
    
    void ExtractGeometryFromWorld(ecs::World& world, const std::string& layerName = "");
    void ExtractGeometryFromEntity(ecs::World& world, ecs::Entity entity);
    
    void Clear();
    bool IsEmpty() const { return buildData_.IsEmpty(); }
    
    NavMeshConfig& GetConfig() { return config_; }
    NavMeshBuildData& GetBuildData() { return buildData_; }
    
    static std::unique_ptr<NavMeshGenerator> Create();
    
private:
    bool Initialize();
    bool BuildGeometry();
    bool BuildNavMesh(NavMesh& navMesh);
    
    NavMeshConfig config_;
    NavMeshBuildData buildData_;
    
    bool initialized_ = false;
    bool hasGeom_ = false;
    
    void* rcContext_ = nullptr;
};

class NavMeshBuilder {
public:
    static NavMeshConfig CreateDefaultConfig();
    
    static void TriangulateBox(const Math::Vec3f& min, const Math::Vec3f& max,
                                std::vector<MeshTriangle>& triangles);
    
    static void TriangulateBox(const Math::Vec3f& center, const Math::Vec3f& halfExtents,
                                const Math::Quatf& rotation, std::vector<MeshTriangle>& triangles);
    
    static void TriangulateCylinder(const Math::Vec3f& center, float radius, float height,
                                     std::vector<MeshTriangle>& triangles, int segments = 16);
    
    static Math::Vec3f CalculateBoundingBoxMin(const std::vector<MeshTriangle>& triangles);
    static Math::Vec3f CalculateBoundingBoxMax(const std::vector<MeshTriangle>& triangles);
};

}
}
