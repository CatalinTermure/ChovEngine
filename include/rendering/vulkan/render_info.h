#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_RENDER_INFO_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_RENDER_INFO_H_

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace chove::rendering::vulkan {

struct RenderInfo {
  vk::Buffer vertex_buffer;
  vk::Buffer index_buffer;
  void* vertex_buffer_memory{};
  void* index_buffer_memory{};
  glm::mat4 model{};
};

}  // namespace chove::rendering::vulkan

#endif  // CHOVENGINE_INCLUDE_RENDERING_VULKAN_RENDER_INFO_H_
