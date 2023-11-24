#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_SHADER_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_SHADER_H_

#include <filesystem>

#include <glm/glm.hpp>
#include <GL/glew.h>

namespace chove::rendering::opengl {
enum class ShaderFlags : uint64_t {
  kNoDiffuseTexture = 1 << 0,
};

class Shader {
 public:
  Shader(const std::filesystem::path &vertex_shader_path, ShaderFlags vertex_shader_flags, const std::filesystem::path &fragment_shader_path, ShaderFlags fragment_shader_flags);
  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&) noexcept;
  Shader &operator=(Shader &&) noexcept;

  void Use() const;
  [[nodiscard]] GLuint program() const { return program_; };

  ~Shader();

 private:
  GLuint program_;
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_SHADER_H_
