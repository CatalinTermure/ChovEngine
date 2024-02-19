#include "rendering/vulkan/context.h"

#include <vulkan/vulkan.hpp>

namespace chove::rendering::vulkan {

Context::Context(vk::Device device, vk::Instance instance) : device(device), instance(instance) {}

Context::Context(Context &&other) noexcept {
  *this = std::move(other);
}
Context &Context::operator=(Context &&other) noexcept {
  device = other.device;
  instance = other.instance;

  other.device = VK_NULL_HANDLE;
  other.instance = VK_NULL_HANDLE;

  return *this;
}
Context::~Context() {
  if (device) {
    device.destroy();
  }
  if (instance) {
    instance.destroy();
  }
}
} // namespace chove::rendering::vulkan
