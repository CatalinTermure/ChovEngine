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

Texture::Texture(const std::filesystem::path &path, std::string name, TextureAllocator &allocator)
    : name_(std::move(name)), allocator_(&allocator) {
  texture_ = allocator_->AllocateTexture(path);
}
}
