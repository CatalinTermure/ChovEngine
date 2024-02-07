#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_

#include "rendering/renderer.h"
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
 private:
  explicit VulkanRenderer(vk::Instance instance, vk::SurfaceKHR surface, vk::Device device);

  vk::Instance instance_ = VK_NULL_HANDLE;
  vk::SurfaceKHR surface_ = VK_NULL_HANDLE;
  vk::Device device_ = VK_NULL_HANDLE;
};
} // namespace chove::rendering::vulkan

#endif //CHOVENGINE_INCLUDE_RENDERING_VULKAN_VULKAN_RENDERER_H_
