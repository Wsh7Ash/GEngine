#include "WebGL2Texture.h"
#include "../../debug/log.h"
#include "../../platform/VFS.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "stb_image.h"

namespace ge {
namespace renderer {

WebGL2Texture::WebGL2Texture(const std::string& path)
    : path_(path)
{
    stbi_set_flip_vertically_on_load(true);

    int width_temp, height_temp, channels_temp;
    uint8_t* data = stbi_load(path.c_str(), &width_temp, &height_temp, &channels_temp, 0);

    if (data) {
        width_ = static_cast<uint32_t>(width_temp);
        height_ = static_cast<uint32_t>(height_temp);
        channels_ = static_cast<uint32_t>(channels_temp);

        glGenTextures(1, &rendererID_);
        glBindTexture(GL_TEXTURE_2D, rendererID_);

        GLenum format = channels_ == 4 ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width_, height_, 0, format, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        GE_LOG_INFO("Loaded texture: {0} ({1}x{2})", path, width_, height_);
    } else {
        GE_LOG_ERROR("Failed to load texture: {0}", path);
        width_ = 1;
        height_ = 1;
    }
}

WebGL2Texture::WebGL2Texture(uint32_t width, uint32_t height, void* data, uint32_t size)
    : width_(width), height_(height)
{
    (void)size;
    glGenTextures(1, &rendererID_);
    glBindTexture(GL_TEXTURE_2D, rendererID_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
}

WebGL2Texture::~WebGL2Texture()
{
    if (rendererID_) {
        glDeleteTextures(1, &rendererID_);
    }
}

void WebGL2Texture::Bind(uint32_t slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, rendererID_);
}

void WebGL2Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace renderer
} // namespace ge
