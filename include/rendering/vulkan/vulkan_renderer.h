#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_

#include "rendering/renderer.h"
#include "rendering/vulkan/allocator.h"
#include "windowing/window.h"

#include <absl/status/statusor.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace chove::rendering::vulkan {
class VulkanRenderer : public Renderer {
 public:
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&other) noexcept;
  VulkanRenderer &operator=(VulkanRenderer &&other) noexcept;

  static VulkanRenderer Create(windowing::Window &window);

  void Render() override;
  void SetupScene(const objects::Scene &scene) override;

  ~VulkanRenderer() override;

  static constexpr int kMaxFramesInFlight = 3;
 private:
  struct RenderAttachments {
    vk::ImageView color_attachment_view;
    vk::ImageView depth_attachment_view;
    vk::Framebuffer framebuffer;
  };

  VulkanRenderer(windowing::Window *window,
                 vk::Instance instance,
                 vk::SurfaceKHR surface,
                 vk::PhysicalDevice physical_device,
                 vk::Device device,
                 uint32_t graphics_queue_family_index,
                 vk::CommandPool graphics_command_pool,
                 vk::RenderPass render_pass,
                 vk::SwapchainKHR swapchain,
                 Allocator allocator,
                 std::array<RenderAttachments, kMaxFramesInFlight> render_attachments);

  vk::Instance instance_ = VK_NULL_HANDLE;
  vk::SurfaceKHR surface_ = VK_NULL_HANDLE;
  vk::Device device_ = VK_NULL_HANDLE;
  vk::PhysicalDevice physical_device_ = VK_NULL_HANDLE;
  vk::Queue graphics_queue_ = VK_NULL_HANDLE;
  uint32_t graphics_queue_family_index_ = 0;
  vk::CommandPool graphics_command_pool_ = VK_NULL_HANDLE;
  vk::RenderPass render_pass_ = VK_NULL_HANDLE;
  vk::SwapchainKHR swapchain_ = VK_NULL_HANDLE;
  Allocator allocator_{};
  windowing::Window *window_ = nullptr;

  std::array<RenderAttachments, kMaxFramesInFlight> render_attachments_;

  static std::array<RenderAttachments, kMaxFramesInFlight>
  CreateFramebuffers(const windowing::WindowExtent &window_extent,
                     const vk::Device &device,
                     Allocator &allocator,
                     const vk::RenderPass &render_pass,
                     vk::SwapchainKHR &swapchain);
};
} // namespace chove::rendering::vulkan

#endif //CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
