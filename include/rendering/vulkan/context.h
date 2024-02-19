#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_CONTEXT_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_CONTEXT_H_

#include <vulkan/vulkan.hpp>

namespace chove::rendering::vulkan {
struct Context {
  Context() = default;
  Context(vk::Device device, vk::Instance instance);
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;
  Context(Context &&other) noexcept;
  Context &operator=(Context &&other) noexcept;
  ~Context();

  vk::Device device = VK_NULL_HANDLE;
  vk::Instance instance = VK_NULL_HANDLE;
};
} // namespace chove::rendering::vulkan

#endif //CHOVENGINE_INCLUDE_RENDERING_VULKAN_CONTEXT_H_
