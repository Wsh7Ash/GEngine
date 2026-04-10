#pragma once

// ================================================================
//  ImageComparator.h
//  Compares images for regression testing.
// ================================================================

#include <string>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <vector>

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
    static ComparisonResult ComparePixels(
        const std::vector<uint8_t>& goldenPixels,
        const std::vector<uint8_t>& outputPixels
    ) {
        const size_t sampleCount = (std::min)(goldenPixels.size(), outputPixels.size());
        const size_t comparedChannels = sampleCount;
        const size_t extraChannels =
            goldenPixels.size() > outputPixels.size()
                ? goldenPixels.size() - outputPixels.size()
                : outputPixels.size() - goldenPixels.size();

        float totalDifference = 0.0f;
        for (size_t i = 0; i < sampleCount; ++i) {
            totalDifference += std::abs(static_cast<int>(goldenPixels[i]) -
                                        static_cast<int>(outputPixels[i])) / 255.0f;
        }

        totalDifference += static_cast<float>(extraChannels);

        const size_t totalChannels = comparedChannels + extraChannels;
        const float normalizedDifference =
            totalChannels > 0 ? totalDifference / static_cast<float>(totalChannels) : 0.0f;

        ComparisonResult result{};
        result.similarityPercent = 1.0f - normalizedDifference;
        result.differentPixels = static_cast<uint32_t>(std::round(totalDifference));
        result.totalPixels = static_cast<uint32_t>(totalChannels);
        return result;
    }

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
