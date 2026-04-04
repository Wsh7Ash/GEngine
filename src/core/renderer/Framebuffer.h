#pragma once

#include <memory>
#include <vector>

namespace ge {
namespace renderer {

/**
 * @brief Configuration for a Framebuffer.
 */
enum class FramebufferTextureFormat {
  None = 0,

  // Color
  RGBA8,
  RGBA16F,
  RG16F,
  RED8,
  RED_INTEGER,

  // Depth/stencil
  DEPTH24STENCIL8,

  // Defaults
  Color = RGBA8,
  Depth = DEPTH24STENCIL8
};

enum class TextureFilter {
  Nearest = 0,
  Linear = 1,
  NearestMipmapNearest = 2,
  LinearMipmapNearest = 3,
  NearestMipmapLinear = 4,
  LinearMipmapLinear = 5
};

enum class TextureWrap {
  Repeat = 0,
  ClampToEdge = 1,
  ClampToBorder = 2,
  MirrorRepeat = 3
};

struct FramebufferTextureSpecification {
  FramebufferTextureSpecification() = default;
  FramebufferTextureSpecification(FramebufferTextureFormat format)
      : TextureFormat(format) {}

  FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
  
  TextureFilter MinFilter = TextureFilter::Linear;
  TextureFilter MagFilter = TextureFilter::Linear;
  TextureWrap WrapU = TextureWrap::ClampToEdge;
  TextureWrap WrapV = TextureWrap::ClampToEdge;
  TextureWrap WrapR = TextureWrap::ClampToEdge;
};

struct FramebufferAttachmentSpecification {
  FramebufferAttachmentSpecification() = default;
  FramebufferAttachmentSpecification(
      std::initializer_list<FramebufferTextureSpecification> attachments)
      : Attachments(attachments) {}

  std::vector<FramebufferTextureSpecification> Attachments;
};

/**
 * @brief Configuration for a Framebuffer.
 */
struct FramebufferSpecification {
  uint32_t Width, Height;
  FramebufferAttachmentSpecification Attachments;
  uint32_t Samples = 1;
  bool SwapChainTarget = false;
};

/**
 * @brief Abstraction for a rendering target off-screen.
 */
class Framebuffer {
public:
  virtual ~Framebuffer() = default;

  virtual void Bind() = 0;
  virtual void Unbind() = 0;

  virtual void Resize(uint32_t width, uint32_t height) = 0;

  virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;
  virtual uint32_t GetDepthAttachmentRendererID() const = 0;
  virtual uint32_t GetEntityAttachmentRendererID() const = 0;
  virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;
  virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;
  virtual const FramebufferSpecification &GetSpecification() const = 0;

  static std::shared_ptr<Framebuffer>
  Create(const FramebufferSpecification &spec);
};

} // namespace renderer
} // namespace ge
