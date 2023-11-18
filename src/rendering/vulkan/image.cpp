#include "rendering/vulkan/image.h"

namespace chove::rendering::vulkan {

Image::Image(Image &&other) noexcept
    : image_(other.image_),
      view_(std::move(other.view_)),
      format_(other.format_),
      allocation_(other.allocation_),
      allocator_(other.allocator_) {
  other.image_ = nullptr;
  other.allocation_ = nullptr;
  other.allocator_ = nullptr;
  other.format_ = vk::Format::eUndefined;
}

Image &Image::operator=(Image &&other) noexcept {
  view_ = std::move(other.view_);

  image_ = other.image_;
  allocation_ = other.allocation_;
  allocator_ = other.allocator_;
  format_ = other.format_;

  other.image_ = nullptr;
  other.allocation_ = nullptr;
  other.allocator_ = nullptr;
  other.format_ = vk::Format::eUndefined;

  return *this;
}

Image::Image(VmaAllocator allocator,
             VmaAllocation allocation,
             vk::Format format,
             vk::raii::ImageView image_view,
             VkImage image) :
    allocator_(allocator),
    view_(std::move(image_view)),
    format_(format),
    image_(image),
    allocation_(allocation) {
}

Image Image::CreateImage(const Context &context,
                         VmaAllocator allocator,
                         vk::Format format,
                         vk::ImageAspectFlags aspect_flags,
                         vk::ImageUsageFlags usage,
                         vk::Extent2D extent,
                         uint32_t queue_family_index) {
  VmaAllocation allocation;
  VkImage c_image;
  const VkImageCreateInfo image_create_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = static_cast<VkFormat>(format),
      .extent = vk::Extent3D{extent, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = static_cast<VkImageUsageFlags>(usage),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queue_family_index,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
  };

  constexpr VmaAllocationCreateInfo allocation_create_info{
      .flags = 0,
      .usage = VMA_MEMORY_USAGE_AUTO,
      .requiredFlags = 0,
      .preferredFlags = 0,
      .memoryTypeBits = 0,
      .pool = VK_NULL_HANDLE,
      .pUserData = nullptr,
      .priority = 0 // TODO: investigate potential performance gains from this
  };

  vmaCreateImage(allocator, &image_create_info, &allocation_create_info, &c_image, &allocation, nullptr);

  vk::raii::ImageView image_view = context.device().createImageView(vk::ImageViewCreateInfo{
      vk::ImageViewCreateFlags{},
      c_image,
      vk::ImageViewType::e2D,
      format,
      vk::ComponentMapping{},
      vk::ImageSubresourceRange{aspect_flags, 0, 1, 0, 1}
  });

  return {allocator, allocation, format, std::move(image_view), c_image};
}

Image::~Image() {
  if (allocator_ == nullptr) return;
  vmaDestroyImage(allocator_, image_, allocation_);
  image_ = nullptr;
}
}  // namespace chove::rendering::vulkan
