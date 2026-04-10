#include "catch_amalgamated.hpp"
#include "../src/core/math/mathUtils.h"

using namespace Math;

TEST_CASE("Math Constants", "[math]")
{
    REQUIRE(PI > 3.14159f);
    REQUIRE(PI < 3.14160f);
}

TEST_CASE("Math Clamp", "[math]")
{
    REQUIRE(Clamp(0.5f, 0.0f, 1.0f) == 0.5f);
    REQUIRE(Clamp(-1.0f, 0.0f, 1.0f) == 0.0f);
    REQUIRE(Clamp(2.0f, 0.0f, 1.0f) == 1.0f);
}

TEST_CASE("Math Lerp", "[math]")
{
    REQUIRE(Lerp(0.0f, 10.0f, 0.0f) == 0.0f);
    REQUIRE(Lerp(0.0f, 10.0f, 1.0f) == 10.0f);
    REQUIRE(Lerp(0.0f, 10.0f, 0.5f) == 5.0f);
}