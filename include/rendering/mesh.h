#ifndef LABSEXTRA_INCLUDE_RENDERING_MESH_H_
#define LABSEXTRA_INCLUDE_RENDERING_MESH_H_

#include <vector>
#include <filesystem>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "material.h"
#include "rendering/vulkan/vulkan_renderer.h"

namespace chove::rendering {
struct Mesh {
  struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
  };

  std::vector<Vertex> vertices;
  std::vector<glm::vec3> color;

  std::vector<uint32_t> indices;

  std::shared_ptr<Material> material;

  static std::vector<Mesh> ImportFromObj(const vulkan::VulkanRenderer& renderer, const std::filesystem::path& path);

  [[nodiscard]] static constexpr std::vector<vk::VertexInputAttributeDescription> attributes();
  [[nodiscard]] static constexpr vk::VertexInputBindingDescription binding() {
    return vk::VertexInputBindingDescription{
        0,
        sizeof(Vertex),
        vk::VertexInputRate::eVertex
    };
  }
};
} // namespace chove::rendering

#endif //LABSEXTRA_INCLUDE_RENDERING_MESH_H_
