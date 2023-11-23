#include "rendering/vulkan/texture.h"
#include "rendering/opengl/texture.h"

#include <stb_image.h>
#include <filesystem>

namespace chove::rendering::vulkan {
std::unique_ptr<Texture> Texture::CreateTexture(const Context &context,
                                                VmaAllocator allocator,
                                                uint32_t graphics_queue_family_index,
                                                const std::filesystem::path &path) {
  int width, height, channels;
  stbi_uc *image_data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!image_data) {
    throw std::runtime_error{"Failed to load texture image"};
  }

  Image image = [&]() {
    switch (channels) {
      case 1:
        return Image::CreateImage(context,
                                  allocator,
                                  vk::Format::eR8Srgb,
                                  vk::ImageAspectFlagBits::eColor,
                                  vk::ImageUsageFlagBits::eSampled,
                                  vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
                                  graphics_queue_family_index);
      case 2:
        return Image::CreateImage(context,
                                  allocator,
                                  vk::Format::eR8G8Srgb,
                                  vk::ImageAspectFlagBits::eColor,
                                  vk::ImageUsageFlagBits::eSampled,
                                  vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
                                  graphics_queue_family_index);
      case 3:
        return Image::CreateImage(context,
                                  allocator,
                                  vk::Format::eR8G8B8Srgb,
                                  vk::ImageAspectFlagBits::eColor,
                                  vk::ImageUsageFlagBits::eSampled,
                                  vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
                                  graphics_queue_family_index);
      case 4:
        return Image::CreateImage(context,
                                  allocator,
                                  vk::Format::eR8G8B8A8Srgb,
                                  vk::ImageAspectFlagBits::eColor,
                                  vk::ImageUsageFlagBits::eSampled,
                                  vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
                                  graphics_queue_family_index);
      default:throw std::runtime_error{"Invalid number of channels"};
    }
  }();

  vk::raii::Sampler sampler{context.device(), vk::SamplerCreateInfo{
      vk::SamplerCreateFlags{},
      vk::Filter::eLinear,
      vk::Filter::eLinear,
      vk::SamplerMipmapMode::eLinear,
      vk::SamplerAddressMode::eRepeat,
      vk::SamplerAddressMode::eRepeat,
      vk::SamplerAddressMode::eRepeat,
      0.0f,
      VK_TRUE,
      16.0f,
      VK_FALSE,
      vk::CompareOp::eAlways,
      0.0f,
      0.0f,
      vk::BorderColor::eIntOpaqueBlack,
      VK_FALSE
  }};

  return std::make_unique<Texture>(std::move(image), std::move(sampler));
}
Texture::Texture(Image image, vk::raii::Sampler sampler)
    : image_(std::move(image)), sampler_(std::move(sampler)) {}
} // namespace chove::rendering::vulkan
