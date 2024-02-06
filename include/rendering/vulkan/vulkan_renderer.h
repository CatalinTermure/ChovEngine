#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_

#include "rendering/renderer.h"
#include "windowing/window.h"

#include <absl/status/statusor.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace chove::rendering::vulkan {
class VulkanRenderer : public Renderer {
 public:
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&) noexcept = default;
  VulkanRenderer &operator=(VulkanRenderer &&) noexcept = default;

  static absl::StatusOr<VulkanRenderer> Create(windowing::Window &window);

  void Render() override;
  void SetupScene(const objects::Scene &scene) override;

  ~VulkanRenderer() override;
 private:
  VulkanRenderer() = default;
};
} // namespace chove::rendering::vulkan

#endif //CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
