#include "rendering/mesh.h"

#include <tiny_obj_loader.h>

namespace chove::rendering {

constexpr std::vector<vk::VertexInputAttributeDescription> Mesh::attributes() {
  return {
      vk::VertexInputAttributeDescription{
          0,
          0,
          vk::Format::eR32G32B32A32Sfloat,
          0
      },
      vk::VertexInputAttributeDescription{
          1,
          0,
          vk::Format::eR32G32B32A32Sfloat,
          sizeof(glm::vec4)
      }
  };
}
Mesh Mesh::ImportFromObj(std::filesystem::path path) {
  return Mesh();
}
}  // namespace chove::rendering
