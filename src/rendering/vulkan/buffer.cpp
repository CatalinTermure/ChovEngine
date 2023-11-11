#include "rendering/vulkan/buffer.h"

namespace chove::rendering::vulkan {

Buffer::Buffer(VmaAllocator allocator,
               vk::DeviceSize size,
               vk::BufferUsageFlags buffer_usage,
               vk::MemoryPropertyFlags required_properties,
               uint32_t queue_family_index)
    : allocator_(allocator), size_(size) {

  VkBufferCreateInfo buffer_create_info{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = size,
      .usage = static_cast<uint32_t>(buffer_usage),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queue_family_index
  };

  VmaAllocatorCreateFlags allocation_flags = 0;
  if (static_cast<uint32_t>(required_properties) & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
    allocation_flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocation_flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
  }
  VmaAllocationCreateInfo allocation_create_info{
      .flags = allocation_flags,
      .usage = VMA_MEMORY_USAGE_AUTO,
      .requiredFlags = static_cast<uint32_t>(required_properties),
      .preferredFlags = 0,
      .memoryTypeBits = 0,
      .pool = VK_NULL_HANDLE,
      .pUserData = nullptr,
      .priority = 0 // TODO: investigate potential performance gains from this
  };

  VkResult result = vmaCreateBuffer(allocator_, &buffer_create_info,
                                    &allocation_create_info, &buffer_, &allocation_, nullptr);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer");
  }

  VmaAllocationInfo allocation_info{};
  vmaGetAllocationInfo(allocator_, allocation_, &allocation_info);
  mapped_data_ = allocation_info.pMappedData;
}

Buffer::~Buffer() {
  if (allocator_ == nullptr) return;
  vmaDestroyBuffer(allocator_, buffer_, allocation_);
}

Buffer &Buffer::operator=(Buffer &&other) noexcept {
  allocator_ = other.allocator_;
  allocation_ = other.allocation_;
  buffer_ = other.buffer_;
  size_ = other.size_;
  mapped_data_ = other.mapped_data_;

  other.allocator_ = nullptr;
  other.allocation_ = nullptr;
  other.buffer_ = nullptr;
  other.size_ = 0;
  other.mapped_data_ = nullptr;

  return *this;
}
Buffer::Buffer(Buffer &&other) noexcept:
    allocator_(other.allocator_),
    allocation_(other.allocation_),
    buffer_(other.buffer_),
    size_(other.size_),
    mapped_data_(other.mapped_data_) {
  other.allocator_ = nullptr;
  other.allocation_ = nullptr;
  other.buffer_ = nullptr;
  other.size_ = 0;
  other.mapped_data_ = nullptr;
}

}
