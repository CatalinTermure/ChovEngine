#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_

#include "objects/scene.h"
#include "rendering/renderer.h"
#include "rendering/vulkan/allocator.h"
#include "rendering/vulkan/context.h"
#include "rendering/vulkan/shader.h"
#include "windowing/window.h"

#include <thread>

#include <vulkan/vulkan.hpp>

namespace chove::rendering::vulkan {
class VulkanRenderer final : public Renderer {
 public:
  VulkanRenderer() = delete;
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&other) noexcept;
  VulkanRenderer &operator=(VulkanRenderer &&other) noexcept;

  static VulkanRenderer Create(windowing::Window &window);

  void Render() override;
  void SetupScene(objects::Scene &scene) override;

  ~VulkanRenderer() override;

  static constexpr int kMaxFramesInFlight = 3;

 private:
  void HandleWindowResize();
  void RenderLoop();

  struct RenderAttachments {
    vk::Image depth_attachment;
    vk::ImageView color_attachment_view;
    vk::ImageView depth_attachment_view;
    vk::Framebuffer framebuffer;
  };

  struct SynchronizationInfo {
    vk::Semaphore image_available;
    vk::Semaphore render_finished;
    vk::Fence in_flight_fence;
  };

  VulkanRenderer(
      windowing::Window *window,
      vk::Instance instance,
      vk::SurfaceKHR surface,
      vk::PhysicalDevice physical_device,
      vk::Device device,
      uint32_t graphics_queue_family_index,
      vk::CommandPool graphics_command_pool,
      vk::RenderPass render_pass,
      vk::SwapchainKHR swapchain,
      Allocator allocator,
      const std::array<RenderAttachments, kMaxFramesInFlight> &render_attachments
  );

  Context context_;
  vk::SurfaceKHR surface_ = VK_NULL_HANDLE;
  vk::PhysicalDevice physical_device_ = VK_NULL_HANDLE;

  Allocator allocator_;
  windowing::Window *window_ = nullptr;

  vk::SwapchainKHR swapchain_ = VK_NULL_HANDLE;
  std::array<RenderAttachments, kMaxFramesInFlight> render_attachments_;
  std::array<SynchronizationInfo, kMaxFramesInFlight> synchronization_info_;
  std::array<std::vector<vk::CommandBuffer>, kMaxFramesInFlight> command_buffers_;

  vk::Queue graphics_queue_ = VK_NULL_HANDLE;
  uint32_t graphics_queue_family_index_ = 0;
  vk::CommandPool graphics_command_pool_ = VK_NULL_HANDLE;
  vk::RenderPass render_pass_ = VK_NULL_HANDLE;
  vk::DescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  std::vector<vk::DescriptorSet> descriptor_sets_;

  std::vector<Shader> shaders_;
  std::vector<vk::Pipeline> pipelines_;
  std::vector<vk::PipelineLayout> pipeline_layouts_;

  objects::Scene *scene_ = nullptr;
  bool is_running_ = false;
  bool render_thread_finished_ = false;
  std::thread render_thread_;

  uint32_t current_frame_ = 0;

  static std::array<RenderAttachments, kMaxFramesInFlight> CreateFramebuffers(
      const windowing::WindowExtent &window_extent,
      const vk::Device &device,
      Allocator &allocator,
      const vk::RenderPass &render_pass,
      const vk::SwapchainKHR &swapchain
  );
};
}  // namespace chove::rendering::vulkan

#endif  // CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
