#include "rendering/opengl/uniform.h"

namespace chove::rendering::opengl {

UniformBuffer::UniformBuffer(UniformBuffer &&other) noexcept: buffer_(other.buffer_), binding_(other.binding_) {
  other.buffer_ = 0;
}

UniformBuffer &UniformBuffer::operator=(UniformBuffer &&other) noexcept {
  buffer_ = other.buffer_;
  binding_ = other.binding_;
  other.buffer_ = 0;
  return *this;
}

UniformBuffer::UniformBuffer(size_t size) {
  binding_ = -1;
  glGenBuffers(1, &buffer_);
  glBindBuffer(GL_UNIFORM_BUFFER, buffer_);
  glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(size), nullptr, GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::Bind(GLuint shader_program, const std::string &name, GLint binding) {
  if (binding_ != binding) {
    binding_ = binding;
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_, buffer_);
  }
  GLuint block_index = glGetUniformBlockIndex(shader_program, name.c_str());
  if (block_index == GL_INVALID_INDEX) {
    LOG(WARNING) << "Uniform block index not found for " << name;
  }
  glUniformBlockBinding(shader_program, block_index, binding_);
}

void UniformBuffer::UpdateData(const void *data, size_t size) const {
  glBindBuffer(GL_UNIFORM_BUFFER, buffer_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, static_cast<GLsizeiptr>(size), data);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UniformBuffer::~UniformBuffer() {
  if (buffer_ > 0) {
    glDeleteBuffers(1, &buffer_);
  }
}
}
