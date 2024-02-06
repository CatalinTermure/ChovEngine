#include "rendering/vulkan/vulkan_renderer.h"

namespace chove::rendering::vulkan {

absl::StatusOr<VulkanRenderer> VulkanRenderer::Create(windowing::Window &window) {
  return VulkanRenderer();
}

void VulkanRenderer::Render() {
}

void VulkanRenderer::SetupScene(const objects::Scene &scene) {

}

VulkanRenderer::~VulkanRenderer() {

}
} // namespace chove::rendering::vulkan
