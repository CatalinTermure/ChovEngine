#ifndef LABSEXTRA_INCLUDE_RENDERING_VULKAN_IMAGE_H_
#define LABSEXTRA_INCLUDE_RENDERING_VULKAN_IMAGE_H_

#include "context.h"

#include <vulkan/vulkan_raii.hpp>
#include <vma/vk_mem_alloc.h>

namespace chove::rendering::vulkan {

class Image {
 public:
  Image(const Image &) = delete;
  Image &operator=(const Image &) = delete;
  Image(Image &&) noexcept;
  Image &operator=(Image &&) noexcept;

  static Image CreateImage(const Context &context,
                           VmaAllocator allocator,
                           vk::Format format,
                           vk::ImageAspectFlags aspect_flags,
                           vk::ImageUsageFlags usage,
                           vk::Extent2D extent,
                           uint32_t queue_family_index);

  [[nodiscard]] vk::ImageView view() const { return *view_; }
  [[nodiscard]] vk::Image image() const { return image_; }
  [[nodiscard]] vk::Format format() const { return format_; }

  virtual ~Image();
 private:
  VkImage image_{};
  vk::raii::ImageView view_;
  vk::Format format_{};
  VmaAllocation allocation_{};
  VmaAllocator allocator_{};

  Image(VmaAllocator allocator,
        VmaAllocation allocation,
        vk::Format format,
        vk::raii::ImageView image_view,
        VkImage image);
};

}  // namespace chove::rendering::vulkan

#endif //LABSEXTRA_INCLUDE_RENDERING_VULKAN_IMAGE_H_
