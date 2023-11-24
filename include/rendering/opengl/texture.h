#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_TEXTURE_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_TEXTURE_H_

#include <string>
#include <filesystem>
#include <GL/glew.h>

#include "rendering/opengl/texture_allocator.h"

namespace chove::rendering::opengl {
class Texture {
 public:
  Texture(const std::filesystem::path &path, std::string name, TextureAllocator &allocator);
  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;
  Texture(Texture &&) noexcept;
  Texture &operator=(Texture &&) noexcept;

  [[nodiscard]] GLuint texture() const { return texture_; };
  [[nodiscard]] const std::string &name() const { return name_; };

  ~Texture();

 private:
  GLuint texture_;
  std::string name_;
  TextureAllocator *allocator_;
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_TEXTURE_H_
