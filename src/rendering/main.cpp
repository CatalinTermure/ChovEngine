// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include <fstream>

namespace chove {
struct QueueInfo {
  uint32_t family_index;
  std::vector<float> priorities;
};

class Logging {
 public:
  Logging() = delete;

  enum class Level {
    kInfo,
    kWarning,
    kEsoteric,
    kError,
    kFatal
  };

  static void Log(Logging::Level level, const std::string& message) {
    std::ostream *os = &std::cout;
    if (level == Level::kWarning || level == Level::kEsoteric || level == Level::kError || level == Level::kFatal) {
      os = &std::cerr;
    }

    switch (level) {
      case Level::kInfo:(*os) << "[INFO]: ";
        break;
      case Level::kWarning:(*os) << "[WARNING]: ";
        break;
      case Level::kEsoteric:(*os) << "[ESOTERIC]: ";
        break;
      case Level::kError:(*os) << "[ERROR]: ";
        break;
      case Level::kFatal:(*os) << "[FATAL]: ";
        break;
    }

    (*os) << message << '\n';

    if (level == Level::kFatal) {
      exit(1);
    }
  }
};
}

using chove::Logging;

SDL_Window *InitSDLWindow() {
  // Create an SDL window that supports Vulkan rendering.
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    Logging::Log(Logging::Level::kFatal, "Could not initialize SDL.");
    return nullptr;
  }

  SDL_Window *window =
      SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
  if (window == nullptr) {
    Logging::Log(Logging::Level::kFatal, "Could not create SDL window.");
    return nullptr;
  }
  return window;
}

std::vector<const char *> GetSDLRequiredExtensions(SDL_Window *window) {
  // Get WSI extensions from SDL (we can add more if we like - we just can't
  // remove these)
  unsigned extension_count;
  if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr)) {
    Logging::Log(Logging::Level::kFatal, "Could not get number of required instance extensions from SDL.");
    return {};
  }
  std::vector<const char *> extensions(extension_count);
  if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count,
                                        extensions.data())) {
    Logging::Log(Logging::Level::kFatal, "Could not get names of required instance extensions from SDL.");
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
      .setApiVersion(VK_API_VERSION_1_0);

  const vk::InstanceCreateInfo inst_info =
      vk::InstanceCreateInfo()
          .setFlags(vk::InstanceCreateFlags())
          .setPApplicationInfo(&app_info)
          .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
          .setPpEnabledExtensionNames(extensions.data())
          .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
          .setPpEnabledLayerNames(layers.data());

  return {context, inst_info};
}

vk::raii::PhysicalDevice PickPhysicalDevice(const vk::raii::Instance &instance) {
  std::ofstream log_file("device_log.txt");
  vk::raii::PhysicalDevices physical_devices(instance);
  for (const auto &physical_device : physical_devices) {
    Logging::Log(Logging::Level::kInfo,
                 std::format("Found device: {}.", (const char *) physical_device.getProperties().deviceName));
  }
  for (const auto &physical_device : physical_devices) {
    vk::PhysicalDeviceProperties properties = physical_device.getProperties();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      return physical_device;
    }
  }
  return physical_devices[0];
}

chove::QueueInfo GetQueueFromCapabilities(const vk::PhysicalDevice &physical_device,
                                          vk::QueueFlags capabilities) {
  auto queue_family_properties = physical_device.getQueueFamilyProperties();

  // TODO: Check for surface support?
  uint32_t family_index =
      std::find_if(queue_family_properties.begin(),
                   queue_family_properties.end(),
                   [capabilities](const auto &queue_properties) {
                     return queue_properties.queueFlags & capabilities;
                   }) - queue_family_properties.begin();
  return {family_index, {1.0f}};
}

vk::raii::Device CreateLogicalDevice(const vk::raii::PhysicalDevice &physical_device, chove::QueueInfo queue_info) {
  vk::DeviceQueueCreateInfo graphics_queue_create_info = vk::DeviceQueueCreateInfo{
      vk::DeviceQueueCreateFlags{},
      queue_info.family_index,
      queue_info.priorities
  };

  std::vector<const char *> layers = {};
  std::vector<const char *> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::DeviceCreateInfo device_create_info = vk::DeviceCreateInfo{
      vk::DeviceCreateFlags{},
      1, &graphics_queue_create_info,
      static_cast<uint32_t>(layers.size()), layers.data(),
      static_cast<uint32_t>(extensions.size()), extensions.data(),
      nullptr, nullptr
  };

  return physical_device.createDevice(device_create_info);;
}

vk::CommandPool CreateCommandPool(uint32_t graphics_queue_index, const vk::Device &device) {
  vk::CommandPoolCreateInfo command_pool_create_info = vk::CommandPoolCreateInfo{
      vk::CommandPoolCreateFlags{},
      graphics_queue_index
  };
  vk::CommandPool command_pool = device.createCommandPool(command_pool_create_info);
  return command_pool;
}
std::vector<vk::CommandBuffer> AllocateCommandBuffers(const vk::Device &device, const vk::CommandPool &command_pool) {
  auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo{
      command_pool,
      vk::CommandBufferLevel::ePrimary,
      1
  };
  std::vector<vk::CommandBuffer> command_buffers = device.allocateCommandBuffers(command_buffer_allocate_info);
  return command_buffers;
}

vk::SurfaceFormatKHR GetBGRA8SurfaceFormat(const vk::SurfaceKHR &surface, const vk::PhysicalDevice &physical_device) {
  std::vector<vk::SurfaceFormatKHR> surface_formats = physical_device.getSurfaceFormatsKHR(surface);
  vk::SurfaceFormatKHR surface_format;
  for (const auto &format : surface_formats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb) {
      surface_format = format;
      break;
    }
  }
  if (surface_format.format == vk::Format::eUndefined) {
    throw std::runtime_error("Driver does not support BGRA8 SRGB.");
  }
  return surface_format;
}

vk::PresentModeKHR PickPresentMode(const vk::SurfaceKHR &surface,
                                   const vk::PhysicalDevice &physical_device,
                                   vk::PresentModeKHR preferred_present_mode) {
  vk::PresentModeKHR present_mode = preferred_present_mode;
  std::vector<vk::PresentModeKHR> supported_present_modes = physical_device.getSurfacePresentModesKHR(surface);
  if (!(std::find(supported_present_modes.begin(), supported_present_modes.end(), present_mode)
      != supported_present_modes.end())) {
    present_mode = vk::PresentModeKHR::eFifo;
  }
  return present_mode;
}

vk::Extent2D GetSurfaceExtent(SDL_Window *window,
                              vk::SurfaceCapabilitiesKHR &surface_capabilities) {
  int window_width, window_height;
  SDL_Vulkan_GetDrawableSize(window, &window_width, &window_height);
  uint32_t surface_width = surface_capabilities.currentExtent.width;
  uint32_t surface_height = surface_capabilities.currentExtent.height;
  if (surface_width == (uint32_t) -1 || surface_height == (uint32_t) -1) {
    surface_width = window_width;
    surface_height = window_height;
  }
  if (surface_width != (uint32_t) window_width || surface_height != (uint32_t) window_height) {
    Logging::Log(Logging::Level::kEsoteric,
                 std::format("SDL and Vulkan disagree on surface size. SDL: %dx%d. Vulkan %ux%u",
                             window_width, window_height, surface_width, surface_height));
  }
  vk::Extent2D surface_extent = {surface_width, surface_height};
  surface_capabilities.currentExtent = surface_extent;
  return surface_extent;
}

int main() {
  auto window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>(InitSDLWindow(), SDL_DestroyWindow);

  vk::raii::Context raii_context = vk::raii::Context{};
  vk::raii::Instance instance = CreateInstance(raii_context, window.get());

  VkSurfaceKHR c_surface;
  if (!SDL_Vulkan_CreateSurface(window.get(), *instance, &c_surface)) {
    Logging::Log(Logging::Level::kFatal, "Could not create a Vulkan surface.");
    return 1;
  }
  vk::raii::SurfaceKHR surface(instance, c_surface);

  vk::raii::PhysicalDevice physical_device = PickPhysicalDevice(instance);
  chove::QueueInfo queue_info = GetQueueFromCapabilities(*physical_device, vk::QueueFlagBits::eGraphics);
  if (!physical_device.getSurfaceSupportKHR(queue_info.family_index, *surface)) {
    Logging::Log(Logging::Level::kFatal, "Queue does not support drawing on this surface.");
    return 1;
  }
  vk::raii::Device device = CreateLogicalDevice(physical_device, queue_info);
  vk::raii::Queue graphics_queue = device.getQueue(queue_info.family_index, 0);
  vk::PresentModeKHR present_mode = PickPresentMode(*surface, *physical_device, vk::PresentModeKHR::eMailbox);
  vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);

  uint32_t image_count = surface_capabilities.minImageCount + 1;
  if (image_count > surface_capabilities.maxImageCount && surface_capabilities.maxImageCount == 0) {
    image_count = surface_capabilities.maxImageCount;
  }
  vk::Extent2D image_size = GetSurfaceExtent(window.get(), surface_capabilities);

  SDL_Quit();
  return 0;
}

