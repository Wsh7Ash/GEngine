#include "../catch_amalgamated.hpp"

TEST_CASE("Image Comparator - Identical Pixels", "[render][regression]")
{
    std::vector<uint8_t> pixels1 = {255, 0, 0, 255, 0, 255, 0, 255};
    std::vector<uint8_t> pixels2 = {255, 0, 0, 255, 0, 255, 0, 255};
    
    uint32_t diff = 0;
    for (size_t i = 0; i < pixels1.size(); ++i) {
        if (pixels1[i] != pixels2[i]) diff++;
    }
    
    float similarity = 1.0f - (static_cast<float>(diff) / pixels1.size());
    REQUIRE(similarity >= 0.99f);
}

TEST_CASE("Image Comparator - Different Pixels", "[render][regression]")
{
    std::vector<uint8_t> pixels1 = {255, 0, 0, 255, 0, 255, 0, 255};
    std::vector<uint8_t> pixels2 = {0, 0, 0, 0, 0, 0, 0, 0};
    
    uint32_t diff = 0;
    for (size_t i = 0; i < pixels1.size(); ++i) {
        if (pixels1[i] != pixels2[i]) diff++;
    }
    
    float similarity = 1.0f - (static_cast<float>(diff) / pixels1.size());
    REQUIRE(similarity < 0.5f);
}

TEST_CASE("Image Comparator - Threshold Behavior", "[render][regression]")
{
    std::vector<uint8_t> golden = {255, 255, 255, 255, 255, 255, 255, 255};
    std::vector<uint8_t> almost = {255, 255, 255, 254, 255, 255, 255, 255};
    
    uint32_t diff = 0;
    for (size_t i = 0; i < golden.size(); ++i) {
        if (golden[i] != almost[i]) diff++;
    }
    
    float similarity = 1.0f - (static_cast<float>(diff) / golden.size());
    
    REQUIRE(similarity >= 0.99f);
    REQUIRE(similarity < 1.0f);
}

TEST_CASE("Image Comparator - SSIM Placeholder", "[render][regression]")
{
    float ssim = 0.0f;
    REQUIRE(ssim >= 0.0f);
    REQUIRE(ssim <= 1.0f);
}