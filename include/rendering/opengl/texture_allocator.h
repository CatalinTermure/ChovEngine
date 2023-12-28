#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_TEXTUREALLOCATOR_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_TEXTUREALLOCATOR_H_

#include <filesystem>
#include <absl/container/flat_hash_map.h>
#include <GL/glew.h>

namespace chove::rendering::opengl {
class TextureAllocator {
 public:
  TextureAllocator();
  ~TextureAllocator();

  TextureAllocator(const TextureAllocator &) = delete;
  TextureAllocator &operator=(const TextureAllocator &) = delete;
  TextureAllocator(TextureAllocator &&) noexcept = delete;
  TextureAllocator &operator=(TextureAllocator &&) noexcept = delete;

  GLuint AllocateTexture(const std::filesystem::path &path);
  GLuint AllocateDepthMap(int width, int height);
  GLuint AllocateCubeDepthMap(int cube_length);
  void DeallocateTexture(GLuint texture);
 private:
  void InvalidateCache();
  void AllocateUnmappedTextureBlockIfNeeded();

  std::vector<GLuint> unmapped_textures_;
  absl::flat_hash_map<std::filesystem::path, GLuint, std::hash<std::filesystem::path>> texture_creation_cache_;
  absl::flat_hash_map<GLuint, uint32_t> texture_ref_counts_;
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_TEXTUREALLOCATOR_H_
