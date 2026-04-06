#pragma once

// ================================================================
//  ImageComparator.h
//  Compares images for regression testing.
// ================================================================

#include <string>
#include <cstdint>

namespace ge {
namespace testing {

struct ComparisonResult {
    float similarityPercent;
    uint32_t differentPixels;
    uint32_t totalPixels;
    std::string outputPath;
    
    bool passed(float threshold) const {
        return similarityPercent >= threshold;
    }
};

class ImageComparator {
public:
    static ComparisonResult Compare(
        const std::string& goldenPath,
        const std::string& outputPath,
        float threshold = 0.99f
    );
    
    static float ComputeSSIM(
        const uint8_t* img1,
        const uint8_t* img2,
        int width, int height
    );
    
    static bool LoadImage(
        const std::string& path,
        int& width, int& height,
        int& channels,
        std::vector<uint8_t>& pixels
    );
    
    static void GenerateDiff(
        const std::string& goldenPath,
        const std::string& outputPath,
        const std::string& diffPath
    );
};

} // namespace testing
} // namespace ge