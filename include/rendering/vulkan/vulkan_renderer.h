#ifndef LABSEXTRA_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
#define LABSEXTRA_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_

#include "rendering/renderer.h"
#include "rendering/window.h"
#include "context.h"
#include "swapchain.h"

#include <absl/status/statusor.h>

#include <vulkan/vulkan_raii.hpp>

namespace chove::rendering::vulkan {

class VulkanRenderer : public Renderer {
 public:
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&) = default;
  VulkanRenderer &operator=(VulkanRenderer &&) = default;

  static absl::StatusOr<VulkanRenderer> Create(Window &window);

  void Render(const objects::Scene &scene) override;
  void SetupScene(const objects::Scene &scene) override;

 private:
  VulkanRenderer(Context context, Swapchain swapchain);

  Context context_;
  Swapchain swapchain_;

  vk::raii::Semaphore image_acquired_semaphore_;
  vk::raii::Semaphore rendering_complete_semaphore_;
  vk::raii::Fence image_presented_fence_;
  vk::raii::CommandPool command_pool_;
  std::vector<vk::raii::CommandBuffer> command_buffers_;

  struct Buffer {
    const Mesh *mesh;
    vk::raii::Buffer buffer;
    vk::raii::DeviceMemory memory;
    vk::DeviceSize offset;
    vk::DeviceSize size;
  };
  std::vector<Buffer> vertex_buffers_;

  std::vector<vk::raii::Pipeline> pipelines_;
  std::vector<vk::raii::PipelineLayout> pipeline_layouts_;
};

}

#endif //LABSEXTRA_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
