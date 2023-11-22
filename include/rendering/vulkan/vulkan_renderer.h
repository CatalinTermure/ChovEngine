#ifndef LABSEXTRA_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
#define LABSEXTRA_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_

#include "rendering/renderer.h"
#include "rendering/window.h"
#include "context.h"
#include "swapchain.h"
#include "buffer.h"
#include "image.h"
#include "allocator.h"
#include "texture.h"

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

  [[nodiscard]] const Context &context() const { return context_; }
  [[nodiscard]] VmaAllocator allocator() const { return gpu_memory_allocator_.allocator(); }

  ~VulkanRenderer() override;

 private:
  VulkanRenderer(Context context,
                 Swapchain swapchain,
                 Image depth_buffer,
                 vk::raii::Semaphore image_acquired_semaphore,
                 vk::raii::Semaphore rendering_complete_semaphore,
                 vk::raii::Semaphore transfer_complete_semaphore,
                 vk::raii::Fence rendering_complete_fence,
                 vk::raii::Fence transfer_complete_fence,
                 vk::raii::CommandPool rendering_command_pool,
                 vk::raii::CommandPool transfer_command_pool,
                 std::vector<vk::raii::CommandBuffer> command_buffers,
                 Allocator gpu_memory_allocator);

  Context context_;
  Allocator gpu_memory_allocator_;

  Swapchain swapchain_;
  Image depth_buffer_;

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
    Buffer uniform_buffer;
    std::unique_ptr<Texture> diffuse_texture;
    const objects::Transform *transform;
    int pipeline_index;
  };
  std::vector<RenderObject> objects_;

  std::vector<vk::raii::Pipeline> pipelines_;
  std::vector<vk::raii::PipelineLayout> pipeline_layouts_;
  std::vector<vk::raii::DescriptorPool> descriptor_pools_;
  std::vector<vk::DescriptorSetLayout> descriptor_set_layouts_;
  std::vector<vk::raii::DescriptorSet> descriptor_sets_;
};
} // namespace chove::rendering::vulkan

#endif //LABSEXTRA_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
