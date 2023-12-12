#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_UNIFORM_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_UNIFORM_H_

#include <absl/log/log.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

namespace chove::rendering::opengl {
template<typename T>
class Uniform {
 public:
  Uniform() {
    LOG(FATAL) << "Uniform not implemented for type.";
  }

  void UpdateValue() {
    LOG(FATAL) << "Uniform not implemented for type.";
  }
};

template<>
class Uniform<glm::mat4> {
 public:
  Uniform() = default;
  Uniform(GLuint shader_program, const std::string &name, const glm::mat4 &value) : value_(value) {
    location_ = glGetUniformLocation(shader_program, name.c_str());
    if (location_ == -1) {
      LOG(WARNING) << "Uniform location not found for " << name;
    }
  }

  void UpdateValue(glm::mat4 value) {
    value_ = value;
    glUniformMatrix4fv(location_, 1, GL_FALSE, glm::value_ptr(value));
  }
 private:
  glm::mat4 value_;
  GLint location_;
};


template<>
class Uniform<glm::mat3> {
 public:
  Uniform() = default;
  Uniform(GLuint shader_program, const std::string &name, const glm::mat3 &value) : value_(value) {
    location_ = glGetUniformLocation(shader_program, name.c_str());
    if (location_ == -1) {
      LOG(WARNING) << "Uniform location not found for " << name;
    }
  }

  void UpdateValue(glm::mat3 value) {
    value_ = value;
    glUniformMatrix3fv(location_, 1, GL_FALSE, glm::value_ptr(value));
  }
 private:
  glm::mat3 value_;
  GLint location_;
};

template<>
class Uniform<glm::vec3> {
 public:
  Uniform() = default;
  Uniform(GLuint shader_program, const std::string &name, const glm::vec3 &value) : value_(value) {
    location_ = glGetUniformLocation(shader_program, name.c_str());
    if (location_ == -1) {
      LOG(WARNING) << "Uniform location not found for " << name;
    }
  }

  void UpdateValue(glm::vec3 value) {
    value_ = value;
    glUniform3fv(location_, 1, glm::value_ptr(value));
  }
 private:
  glm::vec3 value_;
  GLint location_;
};

template<>
class Uniform<glm::vec4> {
 public:
  Uniform() = default;
  Uniform(GLuint shader_program, const std::string &name, const glm::vec4 &value) : value_(value) {
    location_ = glGetUniformLocation(shader_program, name.c_str());
    if (location_ == -1) {
      LOG(WARNING) << "Uniform location not found for " << name;
    }
  }

  void UpdateValue(glm::vec4 value) {
    value_ = value;
    glUniform4fv(location_, 1, glm::value_ptr(value));
  }
 private:
  glm::vec4 value_;
  GLint location_;
};

template<>
class Uniform<float> {
 public:
  Uniform() = default;
  Uniform(GLuint shader_program, const std::string &name, const float &value) : value_(value) {
    location_ = glGetUniformLocation(shader_program, name.c_str());
    if (location_ == -1) {
      LOG(WARNING) << "Uniform location not found for " << name;
    }
  }

  void UpdateValue(float value) {
    value_ = value;
    glUniform1f(location_, value_);
  }
 private:
  float value_;
  GLint location_;
};

class UniformBuffer {
 public:
  UniformBuffer() = default;
  UniformBuffer(UniformBuffer &) = delete;
  UniformBuffer &operator=(UniformBuffer &) = delete;
  UniformBuffer(UniformBuffer &&other) noexcept;
  UniformBuffer &operator=(UniformBuffer &&other) noexcept;

  explicit UniformBuffer(size_t size);

  void Bind(GLuint shader_program, const std::string &name, GLint binding);
  void Rebind() const;

  void UpdateData(const void *data, size_t size) const;
  void UpdateSubData(const void *data, size_t offset, size_t size) const;

  ~UniformBuffer();

 private:
  GLuint buffer_;
  GLint binding_;
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_UNIFORM_H_
