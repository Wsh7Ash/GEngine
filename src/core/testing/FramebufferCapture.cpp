#include "testing/FramebufferCapture.h"
#include "../renderer/Framebuffer.h"
#include <fstream>
#include <algorithm>
#include <cstring>

namespace ge {
namespace testing {

#ifdef _WIN32
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#else
#define STB_IMAGE_WRITE_STATIC
#include <stb_image_write.h>
#endif

std::vector<uint8_t> FramebufferCapture::Capture(const renderer::Framebuffer& framebuffer, uint32_t width, uint32_t height)
{
    std::vector<uint8_t> pixels(width * height * 4);
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.GetRendererID());
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    
    FlipVertically(pixels, width, height, 4);
    
    return pixels;
}

bool FramebufferCapture::SavePNG(const std::string& filepath, const std::vector<uint8_t>& pixels, int width, int height)
{
    std::vector<uint8_t> rgba = pixels;
    FlipVertically(rgba, width, height, 4);
    
    int result = stbi_write_png(filepath.c_str(), width, height, 4, rgba.data(), width * 4);
    return result != 0;
}

bool FramebufferCapture::SavePPM(const std::string& filepath, const std::vector<uint8_t>& pixels, int width, int height)
{
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file << "P6\n" << width << "\n" << height << "\n255\n";
    
    std::vector<uint8_t> rgb = pixels;
    FlipVertically(rgb, width, height, 4);
    
    for (size_t i = 0; i < rgb.size(); i += 4) {
        file.put(static_cast<char>(rgb[i]));
        file.put(static_cast<char>(rgb[i + 1]));
        file.put(static_cast<char>(rgb[i + 2]));
    }
    
    return true;
}

void FramebufferCapture::FlipVertically(std::vector<uint8_t>& pixels, int width, int height, int channels)
{
    int rowSize = width * channels;
    std::vector<uint8_t> row(rowSize);
    
    for (int y = 0; y < height / 2; ++y) {
        std::memcpy(row.data(), pixels.data() + y * rowSize, rowSize);
        std::memcpy(pixels.data() + y * rowSize, pixels.data() + (height - 1 - y) * rowSize, rowSize);
        std::memcpy(pixels.data() + (height - 1 - y) * rowSize, row.data(), rowSize);
    }
}

} // namespace testing
} // namespace ge