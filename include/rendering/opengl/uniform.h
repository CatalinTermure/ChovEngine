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
      LOG(ERROR) << "Uniform location not found for " << name;
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
class Uniform<glm::vec3> {
 public:
  Uniform() = default;
  Uniform(GLuint shader_program, const std::string &name, const glm::vec3 &value) : value_(value) {
    location_ = glGetUniformLocation(shader_program, name.c_str());
    if (location_ == -1) {
      LOG(ERROR) << "Uniform location not found for " << name;
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
      LOG(ERROR) << "Uniform location not found for " << name;
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
      LOG(ERROR) << "Uniform location not found for " << name;
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
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_UNIFORM_H_
