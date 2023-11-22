#ifndef LABSEXTRA_INCLUDE_RENDERING_MESH_H_
#define LABSEXTRA_INCLUDE_RENDERING_MESH_H_

#include <vector>
#include <filesystem>

#include <glm/glm.hpp>

#include "material.h"

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
  Material material;

  static std::vector<Mesh> ImportFromObj(const std::filesystem::path& path);
};
} // namespace chove::rendering

#endif //LABSEXTRA_INCLUDE_RENDERING_MESH_H_
