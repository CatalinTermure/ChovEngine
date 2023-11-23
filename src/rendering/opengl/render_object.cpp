#include "rendering/opengl/render_object.h"

namespace chove::rendering::opengl {

RenderObject::RenderObject(RenderObject &&other) noexcept: model(other.model),
                                                           object_index(other.object_index),
                                                           shader_index(other.shader_index),
                                                           vao(other.vao),
                                                           vbo(other.vbo),
                                                           ebo(other.ebo),
                                                           textures(std::move(other.textures)),
                                                           float_uniforms(std::move(other.float_uniforms)),
                                                           vec3_uniforms(std::move(other.vec3_uniforms)) {
  other.vao = 0;
  other.vbo = 0;
  other.ebo = 0;
}
RenderObject &RenderObject::operator=(RenderObject &&other) noexcept {
  model = other.model;
  object_index = other.object_index;
  shader_index = other.shader_index;
  vao = other.vao;
  vbo = other.vbo;
  ebo = other.ebo;
  textures = std::move(other.textures);
  float_uniforms = std::move(other.float_uniforms);
  vec3_uniforms = std::move(other.vec3_uniforms);

  other.vao = 0;
  other.vbo = 0;
  other.ebo = 0;
  return *this;
}
}
