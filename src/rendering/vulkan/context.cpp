#include "rendering/vulkan/context.h"

#include <memory>

#include <vulkan/vulkan_raii.hpp>
#include <SDL2/SDL_vulkan.h>
#include <absl/log/log.h>
#include <absl/status/statusor.h>

namespace chove::rendering::vulkan {

namespace {


std::vector<const char *> GetSDLRequiredExtensions(SDL_Window *window) {
  // Get WSI extensions from SDL (we can add more if we like - we just can't
  // remove these)
  unsigned int extension_count = 0;
  if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr)) {
    LOG(FATAL) << "Could not get number of required instance extensions from SDL.";
    return {};
  }
  std::vector<const char *> extensions(extension_count);
  if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data())) {
    LOG(FATAL) << "Could not get names of required instance extensions from SDL.";
    return {};
  }
  return extensions;
}

std::vector<const char *> GetValidationLayers() {
  std::vector<const char *> layers;
  layers.push_back("VK_LAYER_KHRONOS_validation");
  return layers;
}

vk::raii::Instance CreateInstance(const vk::raii::Context &context, SDL_Window *window) {
  std::vector<const char *> extensions = GetSDLRequiredExtensions(window);
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  const std::vector<const char *> layers = GetValidationLayers();
  const vk::ApplicationInfo app_info = vk::ApplicationInfo()
      .setPApplicationName("Vulkan C++ Windowed Program Template")
      .setApplicationVersion(1)
      .setPEngineName("LunarG SDK")
      .setEngineVersion(1)
      .setApiVersion(VK_API_VERSION_1_3);

  const vk::InstanceCreateInfo inst_info = vk::InstanceCreateInfo()
      .setFlags(vk::InstanceCreateFlags())
      .setPApplicationInfo(&app_info)
      .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
      .setPpEnabledExtensionNames(extensions.data())
      .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
      .setPpEnabledLayerNames(layers.data());

  return {context, inst_info};
}

vk::raii::PhysicalDevice PickPhysicalDevice(const vk::raii::Instance &instance) {
  vk::raii::PhysicalDevices physical_devices(instance);
  for (const auto &physical_device : physical_devices) {
    LOG(INFO)
            << std::format("Found device: {}.", static_cast<const char *>(physical_device.getProperties().deviceName));
  }
  for (const auto &physical_device : physical_devices) {
    const vk::PhysicalDeviceProperties properties = physical_device.getProperties();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      return physical_device;
    }
  }
  return physical_devices[0];
}

QueueInfo GetQueueFromCapabilities(const vk::PhysicalDevice &physical_device,
                                   vk::QueueFlags capabilities,
                                   vk::QueueFlags prohibited_capabilities = {}) {
  auto queue_family_properties = physical_device.getQueueFamilyProperties();

  // TODO: Check for surface support?
  const uint32_t family_index = std::find_if(queue_family_properties.begin(), queue_family_properties.end(),
                                             [capabilities, prohibited_capabilities](const auto &queue_properties) {
                                               return (queue_properties.queueFlags & capabilities) == capabilities
                                                   && (queue_properties.queueFlags & prohibited_capabilities) ==
                                                       vk::QueueFlags{};
                                             }) -
      queue_family_properties.begin();
  return {family_index, {1.0F}, nullptr};
}

vk::raii::Device CreateLogicalDevice(const vk::raii::PhysicalDevice &physical_device,
                                     const std::vector<QueueInfo> &queue_infos) {
  std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
  for (const auto &queue_info : queue_infos) {
    queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags{}, queue_info.family_index,
                                    queue_info.priorities);
  }

  std::vector<const char *> layers = {};
  std::vector<const char *> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::PhysicalDeviceVulkan13Features device_features{};
  device_features.dynamicRendering = true;
  device_features.synchronization2 = true;

  return physical_device.createDevice(vk::DeviceCreateInfo{
      vk::DeviceCreateFlags{}, queue_create_infos, layers,
      extensions, nullptr, &device_features});
}
}

absl::StatusOr<Context> Context::CreateContext(Window &window) {
  vk::raii::Context vulkan_context{};
  vk::raii::Instance instance = CreateInstance(vulkan_context, window);

  VkSurfaceKHR c_surface = VK_NULL_HANDLE;
  if (!SDL_Vulkan_CreateSurface(window, *instance, &c_surface)) {
    LOG(FATAL) << "SDL could not create Vulkan surface.";
    return absl::Status(absl::StatusCode::kInternal, "SDL could not create Vulkan surface.");
  }
  vk::raii::SurfaceKHR surface{instance, c_surface};

  vk::raii::PhysicalDevice physical_device = PickPhysicalDevice(instance);
  QueueInfo graphics_queue = GetQueueFromCapabilities(*physical_device, vk::QueueFlagBits::eGraphics);
  QueueInfo transfer_queue =
      GetQueueFromCapabilities(*physical_device, vk::QueueFlagBits::eTransfer, vk::QueueFlagBits::eGraphics);
  if (!physical_device.getSurfaceSupportKHR(graphics_queue.family_index, *surface)) {
    LOG(FATAL) << "Device queue does not support drawing on this surface.";
    return absl::Status(absl::StatusCode::kInternal, "Device queue does not support drawing on this surface.");
  }
  vk::raii::Device device = CreateLogicalDevice(physical_device, {graphics_queue, transfer_queue});
  graphics_queue.queue = device.getQueue(graphics_queue.family_index, 0);
  transfer_queue.queue = device.getQueue(transfer_queue.family_index, 0);

  return Context{std::move(vulkan_context),
                 std::move(instance),
                 std::move(surface),
                 std::move(device),
                 std::move(physical_device),
                 std::move(transfer_queue),
                 std::move(graphics_queue)};
}
}
