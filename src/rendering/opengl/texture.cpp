#include "rendering/opengl/texture.h"

#include <stb_image.h>
#include <absl/log/log.h>

#include <utility>

namespace chove::rendering::opengl {

Texture::Texture(Texture &&other) noexcept: texture_(other.texture_), name_(std::move(other.name_)) {
  other.texture_ = 0;
}
Texture &Texture::operator=(Texture &&other) noexcept {
  name_ = std::move(other.name_);
  texture_ = other.texture_;

  other.texture_ = 0;
  return *this;
}
Texture::~Texture() {
  if (texture_ != 0) {
    glDeleteTextures(1, &texture_);
  }
}
Texture::Texture(const std::filesystem::path &path, std::string name) : name_(std::move(name)) {
  int width, height, channels;
  stbi_uc *image_data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (image_data == nullptr) {
    LOG(ERROR) << "Failed to load texture " << path;
    return;
  }

  if ((width & (width - 1)) != 0 || (height & (height - 1)) != 0) {
    LOG(ERROR) << "Texture " << path << " is not a power of two";
  }

  int bytes_per_row = width * 4;
  std::vector<char> buffer(bytes_per_row);
  int half_height = height / 2;
  for (int row = 0; row < half_height; ++row) {
    stbi_uc *row0 = image_data + row * bytes_per_row;
    stbi_uc *row1 = image_data + (height - row - 1) * bytes_per_row;
    memcpy(buffer.data(), row0, bytes_per_row);
    memcpy(row0, row1, bytes_per_row);
    memcpy(row1, buffer.data(), bytes_per_row);
  }

  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  stbi_image_free(image_data);
}
}
