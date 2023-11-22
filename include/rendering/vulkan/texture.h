#ifndef LABSEXTRA_INCLUDE_RENDERING_VULKAN_TEXTURE_H_
#define LABSEXTRA_INCLUDE_RENDERING_VULKAN_TEXTURE_H_

#include <filesystem>

#include <vulkan/vulkan_raii.hpp>

#include "rendering/vulkan/image.h"

namespace chove::rendering::vulkan {
class Texture {
 public:
  Texture(Image image, vk::raii::Sampler sampler);
  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;
  Texture(Texture &&) noexcept = default;
  Texture &operator=(Texture &&) noexcept = default;

  static std::unique_ptr<Texture> CreateTexture(const Context &context,
                                                VmaAllocator allocator,
                                                uint32_t graphics_queue_family_index,
                                                const std::filesystem::path &path);

  [[nodiscard]] vk::ImageView view() const { return image_.view(); }
  [[nodiscard]] vk::Sampler sampler() const { return *sampler_; }

  virtual ~Texture() = default;
 private:
  Image image_;
  vk::raii::Sampler sampler_;

};
} // namespace chove::rendering::vulkan

#endif // LABSEXTRA_INCLUDE_RENDERING_VULKAN_TEXTURE_H_
