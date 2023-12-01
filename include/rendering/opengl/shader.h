#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_SHADER_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_SHADER_H_

#include "rendering/opengl/shader_flags.h"
#include "rendering/opengl/shader_allocator.h"

#include <filesystem>

#include <glm/glm.hpp>
#include <GL/glew.h>

namespace chove::rendering::opengl {

class Shader {
 public:
  Shader(const std::filesystem::path &vertex_shader_path,
         ShaderFlags vertex_shader_flags,
         const std::filesystem::path &fragment_shader_path,
         ShaderFlags fragment_shader_flags,
         ShaderAllocator &shader_allocator);
  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&) noexcept;
  Shader &operator=(Shader &&) noexcept;

  void Use() const;
  [[nodiscard]] GLuint program() const { return program_; };

  ~Shader();

 private:
  GLuint program_;
  ShaderAllocator *allocator_;
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_SHADER_H_
