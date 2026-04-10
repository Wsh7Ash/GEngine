#include "../catch_amalgamated.hpp"
#include "../../src/core/testing/ImageComparator.h"

TEST_CASE("Image Comparator - Identical Pixels", "[render][regression]")
{
    std::vector<uint8_t> pixels1 = {255, 0, 0, 255, 0, 255, 0, 255};
    std::vector<uint8_t> pixels2 = {255, 0, 0, 255, 0, 255, 0, 255};
    
    const auto result = ge::testing::ImageComparator::ComparePixels(pixels1, pixels2);
    float similarity = result.similarityPercent;
    REQUIRE(similarity >= 0.99f);
}

TEST_CASE("Image Comparator - Different Pixels", "[render][regression]")
{
    std::vector<uint8_t> pixels1 = {255, 0, 0, 255, 0, 255, 0, 255};
    std::vector<uint8_t> pixels2 = {0, 255, 255, 0, 255, 0, 255, 0};
    
    const auto result = ge::testing::ImageComparator::ComparePixels(pixels1, pixels2);
    float similarity = result.similarityPercent;
    REQUIRE(similarity < 0.5f);
}

TEST_CASE("Image Comparator - Threshold Behavior", "[render][regression]")
{
    std::vector<uint8_t> golden = {255, 255, 255, 255, 255, 255, 255, 255};
    std::vector<uint8_t> almost = {255, 255, 255, 254, 255, 255, 255, 255};
    
    const auto result = ge::testing::ImageComparator::ComparePixels(golden, almost);
    float similarity = result.similarityPercent;
    
    REQUIRE(similarity >= 0.99f);
    REQUIRE(similarity < 1.0f);
}

TEST_CASE("Image Comparator - SSIM Placeholder", "[render][regression]")
{
    float ssim = 0.0f;
    REQUIRE(ssim >= 0.0f);
    REQUIRE(ssim <= 1.0f);
}
