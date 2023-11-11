#ifndef LABSEXTRA_INCLUDE_RENDERING_MESH_H_
#define LABSEXTRA_INCLUDE_RENDERING_MESH_H_

#include <vector>
#include <filesystem>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "material.h"

namespace chove::rendering {
struct Mesh {
  struct Vertex {
    glm::vec4 position;
    glm::vec4 color;
  };
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  static Mesh ImportFromObj(std::filesystem::path path);

  [[nodiscard]] static constexpr std::vector<vk::VertexInputAttributeDescription> attributes();
  [[nodiscard]] static constexpr vk::VertexInputBindingDescription binding() {
    return vk::VertexInputBindingDescription{
        0,
        sizeof(Vertex),
        vk::VertexInputRate::eVertex
    };
  }
};
}  // namespace chove::rendering

#endif //LABSEXTRA_INCLUDE_RENDERING_MESH_H_
