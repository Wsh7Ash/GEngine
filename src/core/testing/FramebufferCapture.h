#pragma once

// ================================================================
//  FramebufferCapture.h
//  Utility for capturing framebuffer contents to images.
// ================================================================

#include "../renderer/Framebuffer.h"
#include <string>
#include <vector>
#include <cstdint>

namespace ge {
namespace testing {

class FramebufferCapture {
public:
    static std::vector<uint8_t> Capture(const renderer::Framebuffer& framebuffer, uint32_t width, uint32_t height);
    
    static bool SavePNG(const std::string& filepath, const std::vector<uint8_t>& pixels, int width, int height);
    
    static bool SavePPM(const std::string& filepath, const std::vector<uint8_t>& pixels, int width, int height);
    
private:
    static void FlipVertically(std::vector<uint8_t>& pixels, int width, int height, int channels);
};

} // namespace testing
} // namespace ge