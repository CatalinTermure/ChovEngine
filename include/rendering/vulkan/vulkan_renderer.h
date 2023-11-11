#ifndef LABSEXTRA_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
#define LABSEXTRA_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_

#include "rendering/renderer.h"
#include "rendering/window.h"
#include "context.h"
#include "swapchain.h"
#include "buffer.h"

#include <absl/status/statusor.h>

#include <vulkan/vulkan_raii.hpp>

#include <vma/vk_mem_alloc.h>

namespace chove::rendering::vulkan {

class VulkanRenderer : public Renderer {
 public:
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&) noexcept;
  VulkanRenderer &operator=(VulkanRenderer &&) noexcept;

  static absl::StatusOr<VulkanRenderer> Create(Window &window);

  void Render(const objects::Scene &scene) override;
  void SetupScene(const objects::Scene &scene) override;

  ~VulkanRenderer() override;

 private:
  VulkanRenderer(Context context, Swapchain swapchain);

  Context context_;
  Swapchain swapchain_;

  vk::raii::Semaphore image_acquired_semaphore_;
  vk::raii::Semaphore rendering_complete_semaphore_;
  vk::raii::Semaphore transfer_complete_semaphore_;
  vk::raii::Fence rendering_complete_fence_;
  vk::raii::Fence transfer_complete_fence_;

  vk::raii::CommandPool rendering_command_pool_;
  vk::raii::CommandPool transfer_command_pool_;
  std::vector<vk::raii::CommandBuffer> command_buffers_;

  [[nodiscard]] vk::raii::CommandBuffer &drawing_command_buffer() { return command_buffers_[0]; }
  [[nodiscard]] vk::raii::CommandBuffer &transfer_command_buffer() { return command_buffers_[1]; }

  struct RenderObject {
    const Mesh *mesh;
    Buffer vertex_buffer;
    Buffer staging_buffer;
    Buffer index_buffer;
    const objects::Transform *transform;
  };
  std::vector<RenderObject> objects_;

  std::vector<vk::raii::Pipeline> pipelines_;
  std::vector<vk::raii::PipelineLayout> pipeline_layouts_;

  VmaAllocator gpu_memory_allocator_{};
};

}

#endif //LABSEXTRA_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
