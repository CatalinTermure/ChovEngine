#include "rendering/vulkan/allocator.h"

namespace chove::rendering::vulkan {
namespace {
constexpr int kPreferredNvidiaBlockSize = 1048576;
}

Allocator::Allocator(const Context &context) {
  VmaAllocatorCreateInfo allocator_create_info = {
      0,
      context.physical_device(),
      *context.device(),
      kPreferredNvidiaBlockSize,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      context.instance(),
      Context::GetVulkanVersion(),
      nullptr
  };
  vmaCreateAllocator(&allocator_create_info, &allocator_);
}

Allocator::Allocator(Allocator &&other) noexcept {
  allocator_ = other.allocator_;
  other.allocator_ = nullptr;
}

Allocator &Allocator::operator=(Allocator &&other) noexcept {
  allocator_ = other.allocator_;
  other.allocator_ = nullptr;
  return *this;
}

Allocator::~Allocator() {
  vmaDestroyAllocator(allocator_);
}
}  // namespace chove::rendering::vulkan
