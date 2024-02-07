#include "rendering/vulkan/vulkan_renderer.h"

#include <absl/log/log.h>

namespace chove::rendering::vulkan {

namespace {

vk::Instance CreateInstance() {
  std::vector<const char *> required_instance_extensions = windowing::Window::GetRequiredVulkanExtensions();
  vk::ApplicationInfo application_info{
      "Demo app",
      VK_MAKE_VERSION(1, 0, 0),
      "ChovEngine",
      VK_MAKE_VERSION(1, 0, 0),
      VK_API_VERSION_1_3
  };
  vk::InstanceCreateInfo instance_create_info{
      vk::InstanceCreateFlags{},
      &application_info,
      nullptr,
      required_instance_extensions
  };
  vk::Instance instance = vk::createInstance(instance_create_info, nullptr);
  return instance;
}

vk::Device CreateDevice(const vk::Instance &instance, vk::SurfaceKHR &surface) {
  std::vector<vk::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();
  if (physical_devices.empty()) {
    throw std::runtime_error("No physical devices found.");
  }
  vk::PhysicalDevice physical_device = physical_devices[0];
  for (const auto &potential_device : physical_devices) {
    vk::PhysicalDeviceProperties properties = potential_device.getProperties();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      LOG(INFO) << "Using discrete GPU: " << properties.deviceName;
      physical_device = potential_device;
      break;
    }
  }

  std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();
  uint32_t graphics_queue_family_index = -1;
  for (uint32_t family_index = 0; family_index < queue_family_properties.size(); ++family_index) {
    if ((queue_family_properties[family_index].queueFlags & vk::QueueFlagBits::eGraphics)
        && physical_device.getSurfaceSupportKHR(family_index, surface) != 0) {
      LOG(INFO) << "Found graphics queue family with presentation support at index " << family_index;
      graphics_queue_family_index = family_index;
      break;
    }
  }
  if (graphics_queue_family_index == -1) {
    throw std::runtime_error("No graphics queue family with presentation support found.");
  }

  std::vector<float> queue_priorities = {1.0F};
  vk::DeviceQueueCreateInfo graphics_queue_create_info{
      vk::DeviceQueueCreateFlags{},
      graphics_queue_family_index,
      queue_priorities
  };

  vk::Device device = physical_device.createDevice(vk::DeviceCreateInfo{
      vk::DeviceCreateFlags{},
      graphics_queue_create_info,
      nullptr,
      nullptr,
      nullptr
  });
  return device;
}

} // namespace

VulkanRenderer VulkanRenderer::Create(windowing::Window &window) {
  vk::Instance instance = CreateInstance();
  vk::SurfaceKHR surface = window.CreateSurface(instance);
  vk::Device device = CreateDevice(instance, surface);

  return VulkanRenderer{instance, surface, device};
}

void VulkanRenderer::Render() {
}

void VulkanRenderer::SetupScene(const objects::Scene &scene) {
}

VulkanRenderer::~VulkanRenderer() {
  if (device_) {
    device_.destroy();
  }
  if (surface_) {
    instance_.destroySurfaceKHR(surface_);
  }
  if (instance_) {
    instance_.destroy();
  }
}

VulkanRenderer::VulkanRenderer(vk::Instance instance, vk::SurfaceKHR surface, vk::Device device)
    : instance_(instance), surface_(surface), device_(device) {
}

VulkanRenderer::VulkanRenderer(VulkanRenderer &&other) noexcept {
  *this = std::move(other);
}

VulkanRenderer &VulkanRenderer::operator=(VulkanRenderer &&other) noexcept {
  if (this != &other) {
    instance_ = other.instance_;
    surface_ = other.surface_;
    device_ = other.device_;
    other.instance_ = VK_NULL_HANDLE;
    other.surface_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
  }
  return *this;
}
} // namespace chove::rendering::vulkan
