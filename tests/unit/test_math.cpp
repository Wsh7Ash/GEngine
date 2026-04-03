#include "catch_amalgamated.hpp"
#include "../src/core/math/mathUtils.h"
#include "../src/core/math/VecTypes.h"
#include "../src/core/math/Mat4x4.h"
#include "../src/core/math/quaternion.h"

#include <cmath>
#include <limits>

TEST_CASE("Math Constants", "[math]")
{
    REQUIRE(Math::PI > 3.14159f);
    REQUIRE(Math::PI < 3.14160f);
    REQUIRE(Math::TWO_PI == Approx(2.0f * Math::PI));
    REQUIRE(Math::HALF_PI == Approx(Math::PI / 2.0f));
    REQUIRE(Math::EPSILON > 0.0f);
    REQUIRE(Math::EPSILON < 0.001f);
    REQUIRE(Math::INFINITY_VAL == std::numeric_limits<float>::infinity());
}

TEST_CASE("Math Clamp", "[math]")
{
    REQUIRE(Math::Clamp(0.5f, 0.0f, 1.0f) == 0.5f);
    REQUIRE(Math::Clamp(-1.0f, 0.0f, 1.0f) == 0.0f);
    REQUIRE(Math::Clamp(2.0f, 0.0f, 1.0f) == 1.0f);
    REQUIRE(Math::Clamp(5, 0, 10) == 5);
    REQUIRE(Math::Clamp(-5, 0, 10) == 0);
    REQUIRE(Math::Clamp(15, 0, 10) == 10);
}

TEST_CASE("Math Lerp", "[math]")
{
    REQUIRE(Math::Lerp(0.0f, 10.0f, 0.0f) == 0.0f);
    REQUIRE(Math::Lerp(0.0f, 10.0f, 1.0f) == 10.0f);
    REQUIRE(Math::Lerp(0.0f, 10.0f, 0.5f) == 5.0f);
    REQUIRE(Math::Lerp(-10.0f, 10.0f, 0.25f) == -5.0f);
}

TEST_CASE("Math ApproxEqual", "[math]")
{
    REQUIRE(Math::ApproxEqual(1.0f, 1.0f));
    REQUIRE(Math::ApproxEqual(1.0f, 1.0000001f));
    REQUIRE_FALSE(Math::ApproxEqual(1.0f, 1.1f));
    REQUIRE(Math::ApproxEqual(0.0f, 0.0f));
}

TEST_CASE("Math Degrees/Radians", "[math]")
{
    REQUIRE(Math::DegreesToRadians(0.0f) == 0.0f);
    REQUIRE(Math::DegreesToRadians(180.0f) == Approx(Math::PI));
    REQUIRE(Math::DegreesToRadians(360.0f) == Approx(Math::TWO_PI));
    REQUIRE(Math::RadiansToDegrees(0.0f) == 0.0f);
    REQUIRE(Math::RadiansToDegrees(Math::PI) == Approx(180.0f));
    REQUIRE(Math::RadiansToDegrees(Math::TWO_PI) == Approx(360.0f));
}

TEST_CASE("Math Min/Max", "[math]")
{
    REQUIRE(Math::Min(1, 2) == 1);
    REQUIRE(Math::Max(1, 2) == 2);
    REQUIRE(Math::Min(2.0f, 1.0f) == 1.0f);
    REQUIRE(Math::Max(2.0f, 1.0f) == 2.0f);
}

TEST_CASE("Vec2 Operations", "[math][vectors]")
{
    Math::Vec2f a{1.0f, 2.0f};
    Math::Vec2f b{3.0f, 4.0f};

    REQUIRE((a + b).x == 4.0f);
    REQUIRE((a + b).y == 6.0f);
    REQUIRE((a - b).x == -2.0f);
    REQUIRE((a - b).y == -2.0f);
    REQUIRE((a * 2.0f).x == 2.0f);
    REQUIRE((a * 2.0f).y == 4.0f);
    REQUIRE(a.Length() == Approx(std::sqrt(5.0f)));
    REQUIRE(a.Dot(b) == 11.0f);
}

TEST_CASE("Vec3 Operations", "[math][vectors]")
{
    Math::Vec3f a{1.0f, 2.0f, 3.0f};
    Math::Vec3f b{4.0f, 5.0f, 6.0f};

    REQUIRE((a + b).x == 5.0f);
    REQUIRE((a + b).y == 7.0f);
    REQUIRE((a + b).z == 9.0f);
    REQUIRE(a.Dot(b) == 32.0f);
    REQUIRE(a.Length() == Approx(std::sqrt(14.0f)));

    auto normalized = a.Normalized();
    REQUIRE(normalized.Length() == Approx(1.0f).margin(0.001f));
}

TEST_CASE("Vec3 Cross Product", "[math][vectors]")
{
    Math::Vec3f a{1.0f, 0.0f, 0.0f};
    Math::Vec3f b{0.0f, 1.0f, 0.0f};
    auto cross = a.Cross(b);

    REQUIRE(cross.x == 0.0f);
    REQUIRE(cross.y == 0.0f);
    REQUIRE(cross.z == 1.0f);
}

TEST_CASE("Vec4 Operations", "[math][vectors]")
{
    Math::Vec4f a{1.0f, 2.0f, 3.0f, 4.0f};
    Math::Vec4f b{5.0f, 6.0f, 7.0f, 8.0f};

    REQUIRE((a + b).w == 12.0f);
    REQUIRE(a.Dot(b) == 70.0f);
}

TEST_CASE("Mat4x4 Identity", "[math][matrix]")
{
    auto identity = Math::Mat4f::Identity();
    REQUIRE(identity.m[0][0] == 1.0f);
    REQUIRE(identity.m[1][1] == 1.0f);
    REQUIRE(identity.m[2][2] == 1.0f);
    REQUIRE(identity.m[3][3] == 1.0f);
    REQUIRE(identity.m[0][1] == 0.0f);
}

TEST_CASE("Mat4x4 Translation", "[math][matrix]")
{
    auto translate = Math::Mat4f::Translation(Math::Vec3f{1.0f, 2.0f, 3.0f});
    REQUIRE(translate.m[3][0] == 1.0f);
    REQUIRE(translate.m[3][1] == 2.0f);
    REQUIRE(translate.m[3][2] == 3.0f);
}

TEST_CASE("Mat4x4 Multiplication", "[math][matrix]")
{
    auto identity = Math::Mat4f::Identity();
    auto translate = Math::Mat4f::Translation(Math::Vec3f{1.0f, 0.0f, 0.0f});
    auto result = identity * translate;

    REQUIRE(result.m[3][0] == 1.0f);
}

TEST_CASE("Mat4x4 Inverse", "[math][matrix]")
{
    auto identity = Math::Mat4f::Identity();
    auto inv = identity.Inverse();
    REQUIRE(inv.m[0][0] == 1.0f);
    REQUIRE(inv.m[1][1] == 1.0f);
}

TEST_CASE("Quaternion Identity", "[math][quaternion]")
{
    Math::Quat q = Math::Quat::Identity();
    REQUIRE(q.w == 1.0f);
    REQUIRE(q.x == 0.0f);
    REQUIRE(q.y == 0.0f);
    REQUIRE(q.z == 0.0f);
}

TEST_CASE("Quaternion From Axis Angle", "[math][quaternion]")
{
    Math::Quat q = Math::Quat::FromAxisAngle(Math::Vec3f{0.0f, 1.0f, 0.0f}, Math::PI);
    REQUIRE(q.w == Approx(0.0f).margin(0.001f));
    REQUIRE(q.x == Approx(0.0f).margin(0.001f));
    REQUIRE(q.y == Approx(1.0f).margin(0.001f));
    REQUIRE(q.z == Approx(0.0f).margin(0.001f));
}

TEST_CASE("Quaternion Multiply", "[math][quaternion]")
{
    Math::Quat identity = Math::Quat::Identity();
    Math::Quat q = Math::Quat::FromAxisAngle(Math::Vec3f{0.0f, 1.0f, 0.0f}, Math::PI / 2.0f);
    auto result = identity * q;

    REQUIRE(result.w == Approx(q.w).margin(0.001f));
    REQUIRE(result.x == Approx(q.x).margin(0.001f));
}

TEST_CASE("Quaternion To Matrix", "[math][quaternion]")
{
    Math::Quat identity = Math::Quat::Identity();
    auto mat = identity.ToMat4();
    REQUIRE(mat.m[0][0] == 1.0f);
    REQUIRE(mat.m[1][1] == 1.0f);
    REQUIRE(mat.m[2][2] == 1.0f);
}
