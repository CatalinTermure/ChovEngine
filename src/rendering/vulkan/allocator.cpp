#include "rendering/vulkan/allocator.h"

namespace chove::rendering::vulkan {

Allocator Allocator::Create(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device) {
  VmaAllocator allocator = VK_NULL_HANDLE;
  VmaAllocatorCreateInfo allocator_create_info = {};
  allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
  allocator_create_info.instance = instance;
  allocator_create_info.physicalDevice = physical_device;
  allocator_create_info.device = device;
  allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
  VkResult result = vmaCreateAllocator(&allocator_create_info, &allocator);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create VMA allocator.");
  }
  return Allocator{allocator};
}

vk::Image Allocator::AllocateImage(vk::ImageCreateInfo image_create_info,
                                   const VmaAllocationCreateInfo &allocation_create_info) {
  VkImageCreateInfo image_create_info_c = image_create_info;
  VkImage image = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
  VkResult result = vmaCreateImage(allocator_, &image_create_info_c, &allocation_create_info,
                                   &image, &allocation, nullptr);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create image.");
  }

  image_allocations_[image] = allocation;

  return image;
}

Allocator::Allocator(Allocator &&other) noexcept {
  *this = std::move(other);
}

Allocator &Allocator::operator=(Allocator &&other) noexcept {
  allocator_ = other.allocator_;
  image_allocations_ = std::move(other.image_allocations_);
  other.allocator_ = VK_NULL_HANDLE;
  return *this;
}

Allocator::Allocator(VmaAllocator allocator) : allocator_(allocator) {}

Allocator::~Allocator() {
  if (allocator_ != VK_NULL_HANDLE) {
    for (const auto &[image, allocation] : image_allocations_) {
      vmaDestroyImage(allocator_, image, allocation);
    }
    vmaDestroyAllocator(allocator_);
  }
}

void Allocator::Deallocate(vk::Image image) {
  auto iter = image_allocations_.find(image);
  if (iter != image_allocations_.end()) {
    vmaDestroyImage(allocator_, image, iter->second);
    image_allocations_.erase(iter);
  }
}

void Allocator::Deallocate(vk::Buffer buffer) {
  auto iter = buffer_allocations_.find(buffer);
  if (iter != buffer_allocations_.end()) {
    vmaDestroyBuffer(allocator_, buffer, iter->second);
    buffer_allocations_.erase(iter);
  }
}

vk::Buffer Allocator::AllocateBuffer(vk::BufferCreateInfo buffer_create_info,
                                     const VmaAllocationCreateInfo &allocation_create_info) {
  VkBufferCreateInfo buffer_create_info_c = buffer_create_info;
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
  VkResult result = vmaCreateBuffer(allocator_, &buffer_create_info_c, &allocation_create_info,
                                    &buffer, &allocation, nullptr);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer.");
  }

  buffer_allocations_[buffer] = allocation;

  return buffer;
}

} // namespace chove::rendering::vulkan
