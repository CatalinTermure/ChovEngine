#include "rendering/opengl/render_object.h"

namespace chove::rendering::opengl {

RenderObject::RenderObject(RenderObject &&other) noexcept: model(other.model),
                                                           shadow_model(other.shadow_model),
                                                           normal_matrix(other.normal_matrix),
                                                           object_index(other.object_index),
                                                           shader_index(other.shader_index),
                                                           vao(other.vao),
                                                           vbo(other.vbo),
                                                           ebo(other.ebo),
                                                           textures(std::move(other.textures)),
                                                           material_data(std::move(other.material_data)) {
  other.vao = 0;
  other.vbo = 0;
  other.ebo = 0;
}

RenderObject &RenderObject::operator=(RenderObject &&other) noexcept {
  model = other.model;
  normal_matrix = other.normal_matrix;
  shadow_model = other.shadow_model;
  object_index = other.object_index;
  shader_index = other.shader_index;
  vao = other.vao;
  vbo = other.vbo;
  ebo = other.ebo;
  textures = std::move(other.textures);
  material_data = std::move(other.material_data);

  other.vao = 0;
  other.vbo = 0;
  other.ebo = 0;
  return *this;
}

RenderObject::~RenderObject() {
  if (vao != 0) {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
  }
}
}
