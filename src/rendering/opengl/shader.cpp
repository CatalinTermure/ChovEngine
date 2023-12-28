#include "rendering/opengl/shader.h"
#include "rendering/opengl/texture_allocator.h"

namespace chove::rendering::opengl {

Shader::Shader(const std::filesystem::path &vertex_shader_path,
               const std::vector<ShaderFlag> &vertex_shader_flags,
               const std::filesystem::path &fragment_shader_path,
               const std::vector<ShaderFlag> &fragment_shader_flags,
               ShaderAllocator &shader_allocator) {
  allocator_ = &shader_allocator;
  program_ = shader_allocator.AllocateShader(vertex_shader_path,
                                             vertex_shader_flags,
                                             fragment_shader_path,
                                             fragment_shader_flags);
}

Shader::Shader(const std::filesystem::path &vertex_shader_path,
               const std::vector<ShaderFlag> &vertex_shader_flags,
               const std::filesystem::path &fragment_shader_path,
               const std::vector<ShaderFlag> &fragment_shader_flags,
               const std::filesystem::path &geometry_shader_path,
               const std::vector<ShaderFlag> &geometry_shader_flags,
               ShaderAllocator &shader_allocator) {
  allocator_ = &shader_allocator;
  program_ = shader_allocator.AllocateShader(vertex_shader_path,
                                             vertex_shader_flags,
                                             fragment_shader_path,
                                             fragment_shader_flags);
}


Shader::Shader(Shader &&other) noexcept: program_(other.program_), allocator_(other.allocator_) {
  other.program_ = 0;
  other.allocator_ = nullptr;
}

Shader &Shader::operator=(Shader &&other) noexcept {
  program_ = other.program_;
  allocator_ = other.allocator_;
  other.program_ = 0;
  other.allocator_ = nullptr;
  return *this;
}

Shader::~Shader() {
  if (program_ != 0) {
    allocator_->DeallocateShader(program_);
  }
}

void Shader::Use() const {
  glUseProgram(program_);
}
}
