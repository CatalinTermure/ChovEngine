#ifndef CHOVENGINE_INCLUDE_RENDERING_OPENGL_SHADER_ALLOCATOR_H_
#define CHOVENGINE_INCLUDE_RENDERING_OPENGL_SHADER_ALLOCATOR_H_

#include "rendering/opengl/shader_flags.h"

#include <filesystem>

#include <GL/glew.h>

#include <absl/hash/hash.h>
#include <absl/container/node_hash_map.h>

namespace chove::rendering::opengl {
class ShaderAllocator {
 public:
  ShaderAllocator() = default;
  ~ShaderAllocator();

  ShaderAllocator(const ShaderAllocator &) = delete;
  ShaderAllocator &operator=(const ShaderAllocator &) = delete;
  ShaderAllocator(ShaderAllocator &&) noexcept = delete;
  ShaderAllocator &operator=(ShaderAllocator &&) noexcept = delete;

  GLuint AllocateShader(const std::filesystem::path &vertex_shader_path,
                        const std::vector<ShaderFlag> &vertex_shader_flags,
                        const std::filesystem::path &fragment_shader_path,
                        const std::vector<ShaderFlag> &fragment_shader_flags);

  GLuint AllocateShader(const std::filesystem::path &vertex_shader_path,
                        const std::vector<ShaderFlag> &vertex_shader_flags,
                        const std::filesystem::path &fragment_shader_path,
                        const std::vector<ShaderFlag> &fragment_shader_flags,
                        const std::filesystem::path &geometry_shader_path,
                        const std::vector<ShaderFlag> &geometry_shader_flags);

  void DeallocateShader(GLuint shader);

 private:
  void InvalidateCache();

  struct ShaderInfo {
    std::filesystem::path vertex_shader_path;
    std::string vertex_shader_flags;
    std::filesystem::path fragment_shader_path;
    std::string fragment_shader_flags;

    // for hash map equality
    friend bool operator==(const ShaderInfo &lhs, const ShaderInfo &rhs) {
      return lhs.vertex_shader_path == rhs.vertex_shader_path &&
          lhs.vertex_shader_flags == rhs.vertex_shader_flags &&
          lhs.fragment_shader_path == rhs.fragment_shader_path &&
          lhs.fragment_shader_flags == rhs.fragment_shader_flags;
    }

    template<typename H>
    friend H AbslHashValue(H hash, const ShaderInfo &shader) {
      return H::combine(std::move(hash), shader.vertex_shader_path, shader.vertex_shader_flags,
                        shader.fragment_shader_path, shader.fragment_shader_flags);
    }
  };

  absl::node_hash_map<ShaderInfo, GLuint> shader_creation_cache_;
  absl::node_hash_map<GLuint, uint32_t> shader_ref_counts;
};
}

#endif //CHOVENGINE_INCLUDE_RENDERING_OPENGL_SHADER_ALLOCATOR_H_
