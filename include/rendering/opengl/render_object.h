#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDER_OBJECT_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDER_OBJECT_H_

#include "rendering/opengl/uniform.h"
#include "rendering/opengl/texture.h"

#include <vector>

#include <absl/container/flat_hash_map.h>
#include <glm/glm.hpp>
#include <GL/glew.h>

namespace chove::rendering::opengl {
struct RenderObject {
  RenderObject() = default;
  RenderObject(RenderObject &) = delete;
  RenderObject &operator=(RenderObject &) = delete;
  RenderObject(RenderObject &&other) noexcept;
  RenderObject &operator=(RenderObject &&other) noexcept;

  Uniform<glm::mat4> model{};
  Uniform<glm::mat4> shadow_model{};
  Uniform<glm::mat3> normal_matrix{};
  size_t object_index{};
  size_t shader_index{};
  GLuint vao{};
  GLuint vbo{};
  GLuint ebo{};
  std::vector<Texture> textures{};
  UniformBuffer material_data{};
  float dist{};

  ~RenderObject();
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDER_OBJECT_H_
