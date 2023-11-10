#include "rendering/vulkan/swapchain.h"

#include <absl/log/log.h>
#include <SDL2/SDL_vulkan.h>

namespace chove::rendering::vulkan {

namespace {

vk::SurfaceFormatKHR GetBGRA8SurfaceFormat(const vk::SurfaceKHR &surface, const vk::PhysicalDevice &physical_device) {
  const std::vector<vk::SurfaceFormatKHR> surface_formats = physical_device.getSurfaceFormatsKHR(surface);
  vk::SurfaceFormatKHR surface_format;
  for (const auto &format : surface_formats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb) {
      surface_format = format;
      break;
    }
  }
  if (surface_format.format == vk::Format::eUndefined) {
    LOG(FATAL) << "Driver does not support BGRA8 SRGB.";
  }
  return surface_format;
}

vk::Extent2D GetSurfaceExtent(SDL_Window *window, vk::SurfaceCapabilitiesKHR &surface_capabilities) {
  int window_width = 0;
  int window_height = 0;
  SDL_Vulkan_GetDrawableSize(window, &window_width, &window_height);
  uint32_t surface_width = surface_capabilities.currentExtent.width;
  uint32_t surface_height = surface_capabilities.currentExtent.height;
  if (surface_width == 0xFFFFFFFFU || surface_height == 0xFFFFFFFFU) {
    surface_width = window_width;
    surface_height = window_height;
  }
  if (surface_width != static_cast<uint32_t>(window_width) || surface_height != static_cast<uint32_t>(window_height)) {
    LOG(FATAL) << std::format("SDL and Vulkan disagree on surface size. SDL: %dx%d. Vulkan %ux%u", window_width,
                              window_height, surface_width, surface_height);
  }
  vk::Extent2D surface_extent = {surface_width, surface_height};
  surface_capabilities.currentExtent = surface_extent;
  return surface_extent;
}

}

std::pair<vk::Result, SwapchainImage> Swapchain::AcquireNextImage(uint64_t timeout,
                                                                  vk::Semaphore semaphore,
                                                                  vk::Fence fence) const {
  auto [result, index] = swapchain_.acquireNextImage(timeout, semaphore, fence);
  return {result, SwapchainImage{*swapchain_image_views_[index], swapchain_images_[index], index}};
}

absl::StatusOr<Swapchain> Swapchain::CreateSwapchain(Context &context, Window &window, uint32_t image_count) {
  const vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;  // no checks needed for FIFO mode
  vk::SurfaceCapabilitiesKHR
      surface_capabilities = context.physical_device().getSurfaceCapabilitiesKHR(context.surface());

  if (image_count < surface_capabilities.minImageCount
      || (image_count > surface_capabilities.maxImageCount && surface_capabilities.maxImageCount != 0)) {
    return absl::InvalidArgumentError("Desired image count is not supported.");
  }
  vk::Extent2D image_size = GetSurfaceExtent(window, surface_capabilities);
  vk::SurfaceFormatKHR surface_format = GetBGRA8SurfaceFormat(context.surface(), context.physical_device());
  vk::raii::SwapchainKHR swapchain_khr{
      context.device(),
      vk::SwapchainCreateInfoKHR{vk::SwapchainCreateFlagsKHR{}, context.surface(), image_count,
                                 surface_format.format,
                                 surface_format.colorSpace, image_size, 1,
                                 vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive,
                                 context.graphics_queue().family_index, surface_capabilities.currentTransform,
                                 vk::CompositeAlphaFlagBitsKHR::eOpaque, present_mode, true, nullptr}};
  image_count = swapchain_khr.getImages().size();
  LOG(INFO) << "Created swapchain with " << image_count << "images.";
  std::vector<vk::Image> swapchain_images = swapchain_khr.getImages();
  std::vector<vk::raii::ImageView> swapchain_image_views;
  swapchain_image_views.reserve(swapchain_images.size());
  for (auto image : swapchain_images) {
    swapchain_image_views.emplace_back(context.device(), vk::ImageViewCreateInfo{
        vk::ImageViewCreateFlags{}, image, vk::ImageViewType::e2D, surface_format.format, vk::ComponentMapping{},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
  }
  return Swapchain{image_size, surface_format, std::move(swapchain_khr), std::move(swapchain_images),
                   std::move(swapchain_image_views)};
}
}
