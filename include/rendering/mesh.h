#ifndef CHOVENGINE_INCLUDE_RENDERING_MESH_H_
#define CHOVENGINE_INCLUDE_RENDERING_MESH_H_

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
    glm::vec3 tangent;
  };
  struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;

    [[nodiscard]] glm::vec3 center() const {
      return (min + max) / 2.0f;
    }
  };
  std::vector<Vertex> vertices;
  std::vector<glm::vec3> color;
  std::vector<uint32_t> indices;
  Material material;
  BoundingBox bounding_box;

  static std::vector<Mesh> ImportFromObj(const std::filesystem::path& path);
};
} // namespace chove::rendering

#endif //CHOVENGINE_INCLUDE_RENDERING_MESH_H_
