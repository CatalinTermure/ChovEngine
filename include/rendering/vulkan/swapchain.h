#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_SWAPCHAIN_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_SWAPCHAIN_H_

#include <absl/status/statusor.h>
#include <vulkan/vulkan_raii.hpp>

#include "context.h"
#include "rendering/window.h"

namespace chove::rendering::vulkan {

struct SwapchainImage {
  vk::ImageView view;
  vk::Image image;
  uint32_t index{};
};

class Swapchain {
 public:
  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;
  Swapchain(Swapchain &&) noexcept = default;
  Swapchain &operator=(Swapchain &&) noexcept = default;

  static absl::StatusOr<Swapchain> CreateSwapchain(Context &context, Window &window, uint32_t image_count);

  [[nodiscard]] std::pair<vk::Result, SwapchainImage> AcquireNextImage(uint64_t timeout,
                                                                       vk::Semaphore semaphore,
                                                                       vk::Fence fence) const;

  [[nodiscard]] vk::Extent2D image_size() const { return image_size_; }
  [[nodiscard]] vk::SurfaceFormatKHR surface_format() const { return surface_format_; }
  [[nodiscard]] vk::SwapchainKHR swapchain_khr() const { return *swapchain_; }
 private:

  Swapchain(const vk::Extent2D image_size,
            const vk::SurfaceFormatKHR surface_format,
            vk::raii::SwapchainKHR swapchain,
            std::vector<vk::Image> swapchain_images,
            std::vector<vk::raii::ImageView> swapchain_image_views)
      : image_size_(image_size), surface_format_(surface_format), swapchain_(std::move(swapchain)),
        swapchain_images_(std::move(swapchain_images)), swapchain_image_views_(std::move(swapchain_image_views)) {}

  vk::Extent2D image_size_;
  vk::SurfaceFormatKHR surface_format_;
  vk::raii::SwapchainKHR swapchain_;
  std::vector<vk::Image> swapchain_images_;
  std::vector<vk::raii::ImageView> swapchain_image_views_;
};
}

#endif //CHOVENGINE_INCLUDE_RENDERING_VULKAN_SWAPCHAIN_H_
