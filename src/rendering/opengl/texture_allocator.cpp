#include "rendering/opengl/texture_allocator.h"

#include <stb_image.h>
#include <absl/log/log.h>

namespace chove::rendering::opengl {
namespace {
constexpr int kTexturesPerAllocation = 64;
}

TextureAllocator::TextureAllocator() {
  AllocateUnmappedTextureBlockIfNeeded();
}

TextureAllocator::~TextureAllocator() {
  std::vector<GLuint> textures;
  for (auto &[texture, _] : texture_ref_counts_) {
    textures.push_back(texture);
  }
  glDeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
}

void TextureAllocator::AllocateUnmappedTextureBlockIfNeeded() {
  if (!unmapped_textures_.empty()) {
    return;
  }
  unmapped_textures_.resize(kTexturesPerAllocation);
  glGenTextures(kTexturesPerAllocation, unmapped_textures_.data());
}

void TextureAllocator::InvalidateCache() {
  texture_creation_cache_.clear();
}

void TextureAllocator::DeallocateTexture(GLuint texture) {
  texture_ref_counts_.at(texture) -= 1;
  if (texture_ref_counts_.at(texture) == 0) {
    texture_ref_counts_.erase(texture);
    glDeleteTextures(1, &texture);
  }
}

GLuint TextureAllocator::AllocateTexture(const std::filesystem::path &path) {
  if (texture_creation_cache_.contains(path) && texture_ref_counts_.contains(texture_creation_cache_.at(path))) {
    // second check is needed because the texture might have been deallocated
    texture_ref_counts_.at(texture_creation_cache_.at(path)) += 1;
    return texture_creation_cache_.at(path);
  }

  AllocateUnmappedTextureBlockIfNeeded();
  GLuint texture = unmapped_textures_.back();
  unmapped_textures_.pop_back();

  int width, height, channels;
  stbi_uc *image_data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (image_data == nullptr) {
    LOG(ERROR) << "Failed to load texture " << path;
    return -1;
  }

  if ((width & (width - 1)) != 0 || (height & (height - 1)) != 0) {
    LOG(ERROR) << "Texture " << path << " is not a power of two";
    return -1;
  }

  // Flip the image vertically because OpenGL expects the origin to be in the bottom left corner
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

  glBindTexture(GL_TEXTURE_2D, texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  stbi_image_free(image_data);

  texture_ref_counts_[texture] = 1;
  texture_creation_cache_[path] = texture;

  return texture;
}

GLuint TextureAllocator::AllocateDepthMap(int width, int height) {
  AllocateUnmappedTextureBlockIfNeeded();
  GLuint texture = unmapped_textures_.back();
  unmapped_textures_.pop_back();

  glBindTexture(GL_TEXTURE_2D, texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

  texture_ref_counts_[texture] = 1;
  return texture;
}
}