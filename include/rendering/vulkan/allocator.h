#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_ALLOCATOR_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_ALLOCATOR_H_

#include <unordered_map>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace chove::rendering::vulkan {

class Allocator {
 public:
  Allocator() = default; // TODO: this seems like not a good idea, need to think about it
  Allocator(const Allocator &) = delete;
  Allocator &operator=(const Allocator &) = delete;
  Allocator(Allocator &&other) noexcept;
  Allocator &operator=(Allocator &&other) noexcept;

  static Allocator Create(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device);

  ~Allocator();
  void DeallocateAll(); // TODO: move this to destructor, this is needed for now because of the way the renderer is implemented

  vk::Image AllocateImage(vk::ImageCreateInfo image_create_info, const VmaAllocationCreateInfo &allocation_create_info);

 private:
  explicit Allocator(VmaAllocator allocator);

  VmaAllocator allocator_ = VK_NULL_HANDLE;

  std::unordered_map<VkImage, VmaAllocation> image_allocations_;
};

} // namespace chove::rendering::vulkan

#endif //CHOVENGINE_INCLUDE_RENDERING_VULKAN_ALLOCATOR_H_
