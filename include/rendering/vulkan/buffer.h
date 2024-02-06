#ifndef CHOVENGINE_SRC_RENDERING_VULKAN_VULKAN_RENDERER_CPP_BUFFER_H_
#define CHOVENGINE_SRC_RENDERING_VULKAN_VULKAN_RENDERER_CPP_BUFFER_H_

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>
#include <absl/status/statusor.h>
#include "swapchain.h"
#include "context.h"
#include "windowing/window.h"
#include "rendering/renderer.h"

namespace chove::rendering::vulkan {
class Buffer {
 public:
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
  Buffer(Buffer &&) noexcept;
  Buffer &operator=(Buffer &&) noexcept;

  Buffer(VmaAllocator allocator,
         vk::DeviceSize size,
         vk::BufferUsageFlags buffer_usage,
         vk::MemoryPropertyFlags required_properties,
         uint32_t queue_family_index);

  [[nodiscard]] vk::Buffer buffer() const { return buffer_; }
  [[nodiscard]] vk::DeviceSize size() const { return size_; }

  [[nodiscard]] void *GetMappedMemory() const { return mapped_data_; };

  ~Buffer();

 private:
  VmaAllocator allocator_;
  VkBuffer buffer_{};
  VmaAllocation allocation_{};
  vk::DeviceSize size_;

  void *mapped_data_ = nullptr;
};
}

#endif //CHOVENGINE_SRC_RENDERING_VULKAN_VULKAN_RENDERER_CPP_BUFFER_H_
