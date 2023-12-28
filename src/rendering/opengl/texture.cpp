#include "rendering/opengl/texture.h"

namespace chove::rendering::opengl {

Texture::Texture(Texture &&other) noexcept
    : texture_(other.texture_), name_(std::move(other.name_)), allocator_(other.allocator_) {
  other.texture_ = 0;
  other.allocator_ = nullptr;
}

Texture &Texture::operator=(Texture &&other) noexcept {
  name_ = std::move(other.name_);
  texture_ = other.texture_;
  allocator_ = other.allocator_;

  other.texture_ = 0;
  other.allocator_ = nullptr;
  return *this;
}

Texture::~Texture() {
  if (texture_ != 0) {
    allocator_->DeallocateTexture(texture_);
  }
}

// TODO: move depth map settings to the texture constructor, instead of the texture allocator

Texture::Texture(const std::filesystem::path &path, std::string name, TextureAllocator &allocator)
    : name_(std::move(name)), allocator_(&allocator) {
  texture_ = allocator_->AllocateTexture(path);
}

Texture::Texture(int width,
                 int height,
                 std::string name,
                 chove::rendering::opengl::TextureAllocator &allocator) : name_(std::move(name)),
                                                                          allocator_(&allocator) {
  texture_ = allocator_->AllocateDepthMap(width, height);
}

Texture::Texture(int cube_length, std::string name, TextureAllocator &allocator) : name_(std::move(name)),
                                                                                   allocator_(&allocator) {
  texture_ = allocator_->AllocateCubeDepthMap(cube_length);
}
}

