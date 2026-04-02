#include "NavMeshGenerator.h"
#include "debug/log.h"

namespace ge {
namespace navigation {

NavMeshConfig NavMeshBuilder::CreateDefaultConfig() {
    NavMeshConfig config;
    config.cellSize = 0.3f;
    config.cellHeight = 0.2f;
    config.agentHeight = 2.0f;
    config.agentRadius = 0.6f;
    config.agentMaxClimb = 1.0f;
    config.agentMaxSlope = 45.0f;
    config.regionMinSize = 8.0f;
    config.regionMergedSize = 20.0f;
    config.edgeMaxLength = 12.0f;
    config.edgeMaxError = 1.3f;
    config.detailSampleDist = 6.0f;
    config.detailSampleMaxError = 1.0f;
    config.tileWidth = 16;
    config.tileHeight = 16;
    return config;
}

void NavMeshBuilder::TriangulateBox(const Math::Vec3f& min, const Math::Vec3f& max,
                                    std::vector<MeshTriangle>& triangles) {
    Math::Vec3f vertices[8] = {
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {max.x, max.y, min.z},
        {min.x, max.y, min.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {max.x, max.y, max.z},
        {min.x, max.y, max.z}
    };
    
    int faces[6][4] = {
        {0, 1, 2, 3},
        {5, 4, 7, 6},
        {4, 0, 3, 7},
        {1, 5, 6, 2},
        {4, 5, 1, 0},
        {3, 2, 6, 7}
    };
    
    for (int face = 0; face < 6; ++face) {
        int* faceVerts = faces[face];
        triangles.push_back(MeshTriangle(vertices[faceVerts[0]], vertices[faceVerts[1]], vertices[faceVerts[2]]));
        triangles.push_back(MeshTriangle(vertices[faceVerts[0]], vertices[faceVerts[2]], vertices[faceVerts[3]]));
    }
}

void NavMeshBuilder::TriangulateBox(const Math::Vec3f& center, const Math::Vec3f& halfExtents,
                                    const Math::Quatf& rotation, std::vector<MeshTriangle>& triangles) {
    Math::Vec3f localVerts[8] = {
        {-halfExtents.x, -halfExtents.y, -halfExtents.z},
        { halfExtents.x, -halfExtents.y, -halfExtents.z},
        { halfExtents.x,  halfExtents.y, -halfExtents.z},
        {-halfExtents.x,  halfExtents.y, -halfExtents.z},
        {-halfExtents.x, -halfExtents.y,  halfExtents.z},
        { halfExtents.x, -halfExtents.y,  halfExtents.z},
        { halfExtents.x,  halfExtents.y,  halfExtents.z},
        {-halfExtents.x,  halfExtents.y,  halfExtents.z}
    };
    
    Math::Vec3f worldVerts[8];
    for (int i = 0; i < 8; ++i) {
        worldVerts[i] = rotation * localVerts[i] + center;
    }
    
    int faces[6][4] = {
        {0, 1, 2, 3},
        {5, 4, 7, 6},
        {4, 0, 3, 7},
        {1, 5, 6, 2},
        {4, 5, 1, 0},
        {3, 2, 6, 7}
    };
    
    for (int face = 0; face < 6; ++face) {
        int* fv = faces[face];
        triangles.push_back(MeshTriangle(worldVerts[fv[0]], worldVerts[fv[1]], worldVerts[fv[2]]));
        triangles.push_back(MeshTriangle(worldVerts[fv[0]], worldVerts[fv[2]], worldVerts[fv[3]]));
    }
}

void NavMeshBuilder::TriangulateCylinder(const Math::Vec3f& center, float radius, float height,
                                         std::vector<MeshTriangle>& triangles, int segments) {
    float halfHeight = height * 0.5f;
    Math::Vec3f bottomCenter = center - Math::Vec3f(0, halfHeight, 0);
    Math::Vec3f topCenter = center + Math::Vec3f(0, halfHeight, 0);
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = (float)i / segments * 2.0f * 3.14159265f;
        float angle2 = (float)(i + 1) / segments * 2.0f * 3.14159265f;
        
        float x1 = center.x + radius * std::cos(angle1);
        float z1 = center.z + radius * std::sin(angle1);
        float x2 = center.x + radius * std::cos(angle2);
        float z2 = center.z + radius * std::sin(angle2);
        
        Math::Vec3f v1 = {x1, center.y - halfHeight, z1};
        Math::Vec3f v2 = {x2, center.y - halfHeight, z2};
        Math::Vec3f v3 = {x2, center.y + halfHeight, z2};
        Math::Vec3f v4 = {x1, center.y + halfHeight, z1};
        
        triangles.push_back(MeshTriangle(v1, v2, v3));
        triangles.push_back(MeshTriangle(v1, v3, v4));
    }
}

Math::Vec3f NavMeshBuilder::CalculateBoundingBoxMin(const std::vector<MeshTriangle>& triangles) {
    if (triangles.empty()) return Math::Vec3f::Zero();
    
    Math::Vec3f min = triangles[0].vertices[0];
    for (const auto& tri : triangles) {
        for (int i = 0; i < 3; ++i) {
            min.x = std::min(min.x, tri.vertices[i].x);
            min.y = std::min(min.y, tri.vertices[i].y);
            min.z = std::min(min.z, tri.vertices[i].z);
        }
    }
    return min;
}

Math::Vec3f NavMeshBuilder::CalculateBoundingBoxMax(const std::vector<MeshTriangle>& triangles) {
    if (triangles.empty()) return Math::Vec3f::Zero();
    
    Math::Vec3f max = triangles[0].vertices[0];
    for (const auto& tri : triangles) {
        for (int i = 0; i < 3; ++i) {
            max.x = std::max(max.x, tri.vertices[i].x);
            max.y = std::max(max.y, tri.vertices[i].y);
            max.z = std::max(max.z, tri.vertices[i].z);
        }
    }
    return max;
}

}
}
