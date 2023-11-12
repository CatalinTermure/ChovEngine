#ifndef LABSEXTRA_INCLUDE_RENDERING_VULKAN_ALLOCATOR_H_
#define LABSEXTRA_INCLUDE_RENDERING_VULKAN_ALLOCATOR_H_

#include <vma/vk_mem_alloc.h>
#include "context.h"

namespace chove::rendering::vulkan {
class Allocator {
 public:
  explicit Allocator(const Context &context);
  Allocator(const Allocator &) = delete;
  Allocator &operator=(const Allocator &) = delete;
  Allocator(Allocator &&) noexcept;
  Allocator &operator=(Allocator &&) noexcept;

  [[nodiscard]] VmaAllocator allocator() const { return allocator_; }

  ~Allocator();
 private:
  VmaAllocator allocator_{};
};
}  // namespace chove::rendering::vulkan

#endif //LABSEXTRA_INCLUDE_RENDERING_VULKAN_ALLOCATOR_H_
