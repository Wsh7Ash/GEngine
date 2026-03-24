#pragma once

#include "VecTypes.h"
#include "Mat4x4.h"
#include <vector>
#include <algorithm>




namespace Math {

    struct AABB {
        Vec3f Min = { FLT_MAX, FLT_MAX, FLT_MAX };
        Vec3f Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        AABB() = default;
        AABB(const Vec3f& min, const Vec3f& max) : Min(min), Max(max) {}

        void Expand(const Vec3f& point) {
            Min.x = std::min(Min.x, point.x);
            Min.y = std::min(Min.y, point.y);
            Min.z = std::min(Min.z, point.z);
            Max.x = std::max(Max.x, point.x);
            Max.y = std::max(Max.y, point.y);
            Max.z = std::max(Max.z, point.z);
        }

        AABB Transform(const Mat4f& transform) const {
            Vec3f corners[8] = {
                { Min.x, Min.y, Min.z }, { Max.x, Min.y, Min.z },
                { Min.x, Max.y, Min.z }, { Max.x, Max.y, Min.z },
                { Min.x, Min.y, Max.z }, { Max.x, Min.y, Max.z },
                { Min.x, Max.y, Max.z }, { Max.x, Max.y, Max.z }
            };

            AABB result;
            for (int i = 0; i < 8; ++i) {
                Vec4f v = transform * Vec4f(corners[i].x, corners[i].y, corners[i].z, 1.0f);
                result.Expand({ v.x, v.y, v.z });
            }
            return result;
        }
    };

    /**
     * @brief Plane in 3D space (Ax + By + Cz + D = 0).
     */
    struct Plane {
        Vec3f Normal = { 0, 1, 0 };
        float Distance = 0;

        Plane() = default;
        Plane(const Vec3f& normal, float distance) : Normal(normal.Normalized()), Distance(distance) {}

        float GetDistance(const Vec3f& point) const {
            return Normal.Dot(point) + Distance;
        }
    };

    /**
     * @brief View Frustum defined by 6 planes.
     */
    class Frustum {
    public:
        enum Side { Left = 0, Right, Bottom, Top, Near, Far };
        Plane Planes[6];

        void FromMatrix(const Mat4f& m) {
            // Left Plane
            Planes[Left].Normal.x = m[3][0] + m[0][0];
            Planes[Left].Normal.y = m[3][1] + m[0][1];
            Planes[Left].Normal.z = m[3][2] + m[0][2];
            Planes[Left].Distance = m[3][3] + m[0][3];

            // Right Plane
            Planes[Right].Normal.x = m[3][0] - m[0][0];
            Planes[Right].Normal.y = m[3][1] - m[0][1];
            Planes[Right].Normal.z = m[3][2] - m[0][2];
            Planes[Right].Distance = m[3][3] - m[0][3];

            // Bottom Plane
            Planes[Bottom].Normal.x = m[3][0] + m[1][0];
            Planes[Bottom].Normal.y = m[3][1] + m[1][1];
            Planes[Bottom].Normal.z = m[3][2] + m[1][2];
            Planes[Bottom].Distance = m[3][3] + m[1][3];

            // Top Plane
            Planes[Top].Normal.x = m[3][0] - m[1][0];
            Planes[Top].Normal.y = m[3][1] - m[1][1];
            Planes[Top].Normal.z = m[3][2] - m[1][2];
            Planes[Top].Distance = m[3][3] - m[1][3];

            // Near Plane
            Planes[Near].Normal.x = m[3][0] + m[2][0];
            Planes[Near].Normal.y = m[3][1] + m[2][1];
            Planes[Near].Normal.z = m[3][2] + m[2][2];
            Planes[Near].Distance = m[3][3] + m[2][3];

            // Far Plane
            Planes[Far].Normal.x = m[3][0] - m[2][0];
            Planes[Far].Normal.y = m[3][1] - m[2][1];
            Planes[Far].Normal.z = m[3][2] - m[2][2];
            Planes[Far].Distance = m[3][3] - m[2][3];

            for (int i = 0; i < 6; i++) {
                float length = Planes[i].Normal.Length();
                Planes[i].Normal /= length;
                Planes[i].Distance /= length;
            }
        }

        bool Intersect(const AABB& aabb) const {
            for (int i = 0; i < 6; i++) {
                // Find positive vertex
                Vec3f p = aabb.Min;
                if (Planes[i].Normal.x >= 0) p.x = aabb.Max.x;
                if (Planes[i].Normal.y >= 0) p.y = aabb.Max.y;
                if (Planes[i].Normal.z >= 0) p.z = aabb.Max.z;

                if (Planes[i].GetDistance(p) < 0) return false;
            }
            return true;
        }
    };

} // namespace Math



