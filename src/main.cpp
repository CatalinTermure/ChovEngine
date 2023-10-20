// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include <filesystem>
#include <queue>

#include "rendering/camera.h"
#include "utils/logging.h"

namespace chove {

struct QueueInfo {
  uint32_t family_index;
  std::vector<float> priorities;
};
}  // namespace chove

using chove::Logging;

SDL_Window *InitSDLWindow() {
  // Create an SDL window that supports Vulkan rendering.
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    Logging::Log(Logging::Level::kFatal, "Could not initialize SDL.");
    return nullptr;
  }

  SDL_Window *window =
      SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
  if (window == nullptr) {
    Logging::Log(Logging::Level::kFatal, "Could not create SDL window.");
    return nullptr;
  }
  return window;
}

std::vector<const char *> GetSDLRequiredExtensions(SDL_Window *window) {
  // Get WSI extensions from SDL (we can add more if we like - we just can't
  // remove these)
  unsigned int extension_count = 0;
  if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr)) {
    Logging::Log(Logging::Level::kFatal, "Could not get number of required instance extensions from SDL.");
    return {};
  }
  std::vector<const char *> extensions(extension_count);
  if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data())) {
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
    Logging::Log(
        Logging::Level::kInfo,
        std::format("Found device: {}.", static_cast<const char *>(physical_device.getProperties().deviceName)));
  }
  for (const auto &physical_device : physical_devices) {
    const vk::PhysicalDeviceProperties properties = physical_device.getProperties();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      return physical_device;
    }
  }
  return physical_devices[0];
}

chove::QueueInfo GetQueueFromCapabilities(const vk::PhysicalDevice &physical_device, vk::QueueFlags capabilities) {
  auto queue_family_properties = physical_device.getQueueFamilyProperties();

  // TODO: Check for surface support?
  const uint32_t family_index = std::find_if(queue_family_properties.begin(), queue_family_properties.end(),
                                             [capabilities](const auto &queue_properties) {
                                               return queue_properties.queueFlags & capabilities;
                                             }) -
      queue_family_properties.begin();
  return {family_index, {1.0F}};
}

vk::raii::Device CreateLogicalDevice(const vk::raii::PhysicalDevice &physical_device, chove::QueueInfo queue_info) {
  const vk::DeviceQueueCreateInfo graphics_queue_create_info =
      vk::DeviceQueueCreateInfo{vk::DeviceQueueCreateFlags{}, queue_info.family_index, queue_info.priorities};

  std::vector<const char *> layers = {};
  std::vector<const char *> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::PhysicalDeviceVulkan13Features device_features{};
  device_features.dynamicRendering = true;
  device_features.synchronization2 = true;

  return physical_device.createDevice(vk::DeviceCreateInfo{
      vk::DeviceCreateFlags{}, 1, &graphics_queue_create_info, static_cast<uint32_t>(layers.size()), layers.data(),
      static_cast<uint32_t>(extensions.size()), extensions.data(), nullptr, &device_features});
}

vk::raii::CommandPool CreateCommandPool(uint32_t graphics_queue_index, const vk::raii::Device &device) {
  vk::raii::CommandPool command_pool =
      device.createCommandPool({vk::CommandPoolCreateFlags{vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
                                graphics_queue_index});
  return command_pool;
}
std::vector<vk::raii::CommandBuffer> AllocateCommandBuffers(const vk::raii::Device &device,
                                                            const vk::raii::CommandPool &command_pool) {
  auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo{*command_pool, vk::CommandBufferLevel::ePrimary, 1};
  std::vector<vk::raii::CommandBuffer> command_buffers = device.allocateCommandBuffers(command_buffer_allocate_info);
  return command_buffers;
}

vk::SurfaceFormatKHR GetBGRA8SurfaceFormat(const vk::SurfaceKHR &surface, const vk::PhysicalDevice &physical_device) {
  const std::vector<vk::SurfaceFormatKHR> surface_formats = physical_device.getSurfaceFormatsKHR(surface);
  vk::SurfaceFormatKHR surface_format;
  for (const auto &format : surface_formats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb) {
      surface_format = format;
      break;
    }
  }
  if (surface_format.format == vk::Format::eUndefined) {
    Logging::Log(Logging::Level::kFatal, "Driver does not support BGRA8 SRGB.");
  }
  return surface_format;
}

vk::PresentModeKHR PickPresentMode(const vk::SurfaceKHR &surface, const vk::PhysicalDevice &physical_device,
                                   vk::PresentModeKHR preferred_present_mode) {
  vk::PresentModeKHR present_mode = preferred_present_mode;
  std::vector<vk::PresentModeKHR> supported_present_modes = physical_device.getSurfacePresentModesKHR(surface);
  if (!(std::find(supported_present_modes.begin(), supported_present_modes.end(), present_mode) !=
      supported_present_modes.end())) {
    Logging::Log(Logging::Level::kWarning, "Present mode not supported. Falling back to FIFO.");
    present_mode = vk::PresentModeKHR::eFifo;
  }
  return present_mode;
}

vk::Extent2D GetSurfaceExtent(SDL_Window *window, vk::SurfaceCapabilitiesKHR &surface_capabilities) {
  int window_width = 0;
  int window_height = 0;
  SDL_Vulkan_GetDrawableSize(window, &window_width, &window_height);
  uint32_t surface_width = surface_capabilities.currentExtent.width;
  uint32_t surface_height = surface_capabilities.currentExtent.height;
  if (surface_width == 0xFFFFFFFFU || surface_height == 0xFFFFFFFFU) {
    surface_width = window_width;
    surface_height = window_height;
  }
  if (surface_width != static_cast<uint32_t>(window_width) || surface_height != static_cast<uint32_t>(window_height)) {
    Logging::Log(Logging::Level::kEsoteric,
                 std::format("SDL and Vulkan disagree on surface size. SDL: %dx%d. Vulkan %ux%u", window_width,
                             window_height, surface_width, surface_height));
  }
  vk::Extent2D surface_extent = {surface_width, surface_height};
  surface_capabilities.currentExtent = surface_extent;
  return surface_extent;
}

vk::raii::ShaderModule GetShaderModule(const std::filesystem::path &shader_file_path, const vk::raii::Device &device) {
  std::ifstream shader_file{shader_file_path, std::ios::binary};
  shader_file.seekg(0, std::ios::end);
  auto shader_file_size = shader_file.tellg();
  shader_file.seekg(0, std::ios::beg);
  std::vector<char> buffer(shader_file_size);
  shader_file.read(buffer.data(), shader_file_size);
  std::vector<uint32_t> shader_code(reinterpret_cast<uint32_t *>(buffer.data()),
                                    reinterpret_cast<uint32_t *>(buffer.data() + shader_file_size));

  return vk::raii::ShaderModule{device, vk::ShaderModuleCreateInfo{
      vk::ShaderModuleCreateFlags{},
      shader_code
  }};
}

const std::vector<glm::vec4> triangle = {{-1.0F, 0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F, 1.0F}};

constexpr int target_frame_rate = 120;
constexpr long long target_frame_time_ns = 1'000'000'000 / target_frame_rate;
constexpr std::chrono::duration target_frame_time = std::chrono::nanoseconds(target_frame_time_ns);

int main() {
  auto window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>(InitSDLWindow(), SDL_DestroyWindow);

  const vk::raii::Context raii_context = vk::raii::Context{};
  const vk::raii::Instance instance = CreateInstance(raii_context, window.get());

  VkSurfaceKHR c_surface = VK_NULL_HANDLE;
  if (!SDL_Vulkan_CreateSurface(window.get(), *instance, &c_surface)) {
    Logging::Log(Logging::Level::kFatal, "Could not create a Vulkan surface.");
    return 1;
  }
  const vk::raii::SurfaceKHR surface(instance, c_surface);

  const vk::raii::PhysicalDevice physical_device = PickPhysicalDevice(instance);
  chove::QueueInfo queue_info = GetQueueFromCapabilities(*physical_device, vk::QueueFlagBits::eGraphics);
  if (!physical_device.getSurfaceSupportKHR(queue_info.family_index, *surface)) {
    Logging::Log(Logging::Level::kFatal, "Queue does not support drawing on this surface.");
    return 1;
  }
  const vk::raii::Device device = CreateLogicalDevice(physical_device, queue_info);
  const vk::raii::Queue graphics_queue = device.getQueue(queue_info.family_index, 0);
  const vk::PresentModeKHR present_mode = PickPresentMode(*surface, *physical_device, vk::PresentModeKHR::eMailbox);
  vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);

  uint32_t image_count = surface_capabilities.minImageCount + 1;
  if (image_count > surface_capabilities.maxImageCount && surface_capabilities.maxImageCount == 0) {
    image_count = surface_capabilities.maxImageCount;
  }
  const vk::Extent2D image_size = GetSurfaceExtent(window.get(), surface_capabilities);
  const vk::SurfaceFormatKHR surface_format = GetBGRA8SurfaceFormat(*surface, *physical_device);
  const vk::raii::SwapchainKHR swapchain_khr{
      device, vk::SwapchainCreateInfoKHR{vk::SwapchainCreateFlagsKHR{}, *surface, image_count, surface_format.format,
                                         surface_format.colorSpace, image_size, 1,
                                         vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive,
                                         queue_info.family_index, surface_capabilities.currentTransform,
                                         vk::CompositeAlphaFlagBitsKHR::eOpaque, present_mode, true, nullptr}};
  image_count = swapchain_khr.getImages().size();
  Logging::Log(Logging::Level::kInfo, std::format("Created swapchain with {} images.", image_count));
  std::vector<vk::Image> swapchain_images = swapchain_khr.getImages();
  std::vector<vk::raii::ImageView> swapchain_image_views;
  for (auto image : swapchain_images) {
    swapchain_image_views.emplace_back(device, vk::ImageViewCreateInfo{
        vk::ImageViewCreateFlags{}, image, vk::ImageViewType::e2D, surface_format.format, vk::ComponentMapping{},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
  }

  const vk::raii::CommandPool command_pool = CreateCommandPool(queue_info.family_index, device);
  const std::vector<vk::raii::CommandBuffer> command_buffers = AllocateCommandBuffers(device, command_pool);

  chove::Camera camera({0.0F, 0.0F, -1.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, glm::radians(60.0F),
                       static_cast<float>(image_size.width) / static_cast<float>(image_size.height), 0.1F, 10.0F);

  glm::mat4 view_matrix = camera.GetTransformMatrix();

  vk::raii::Semaphore image_acquired_semaphore{device, vk::SemaphoreCreateInfo{}};
  vk::raii::Semaphore rendering_complete_semaphore{device, vk::SemaphoreCreateInfo{}};
  vk::raii::Fence image_presented_fence{device, vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}};

  vk::raii::ShaderModule vertex_shader_module = GetShaderModule("shaders/render_shader.vert.spv", device);
  vk::PipelineShaderStageCreateInfo vertex_shader_stage_create_info{
      vk::PipelineShaderStageCreateFlags{},
      vk::ShaderStageFlagBits::eVertex,
      *vertex_shader_module,
      "main"
  };

  vk::raii::ShaderModule fragment_shader_module = GetShaderModule("shaders/render_shader.frag.spv", device);
  vk::PipelineShaderStageCreateInfo fragment_shader_stage_create_info{
      vk::PipelineShaderStageCreateFlags{},
      vk::ShaderStageFlagBits::eFragment,
      *fragment_shader_module,
      "main"
  };

  std::vector<vk::PipelineShaderStageCreateInfo>
      shader_stages{vertex_shader_stage_create_info, fragment_shader_stage_create_info};

  vk::PipelineVertexInputStateCreateInfo vertex_input_state{
      vk::PipelineVertexInputStateCreateFlags{},
      0,
      nullptr,
      0,
      nullptr
  };

  vk::PipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state{
      vk::PipelineInputAssemblyStateCreateFlags{},
      vk::PrimitiveTopology::eTriangleList,
      false
  };

  vk::PipelineTessellationStateCreateInfo pipeline_tessellation_state{
      vk::PipelineTessellationStateCreateFlags{},
      0
  };

  vk::Viewport viewport{
      0.0F,
      0.0F,
      static_cast<float>(image_size.width),
      static_cast<float>(image_size.height),
      0.0F,
      1.0F
  };
  vk::Rect2D scissor{{0, 0}, image_size};
  vk::PipelineViewportStateCreateInfo pipeline_viewport_state{
      vk::PipelineViewportStateCreateFlags{},
      viewport,
      scissor
  };

  vk::PipelineRasterizationStateCreateInfo pipeline_rasterization_state{
      vk::PipelineRasterizationStateCreateFlags{},
      false,
      false,
      vk::PolygonMode::eFill,
      vk::CullModeFlagBits::eNone,
      vk::FrontFace::eCounterClockwise,
      false,
      0.0F,
      0.0F,
      0.0F,
      1.0F,
      nullptr
  };

  vk::PipelineColorBlendAttachmentState color_blend_attachment_state{
      false,
      vk::BlendFactor::eZero,
      vk::BlendFactor::eZero,
      vk::BlendOp::eAdd,
      vk::BlendFactor::eZero,
      vk::BlendFactor::eZero,
      vk::BlendOp::eAdd,
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
          vk::ColorComponentFlagBits::eA
  };

  vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state{
      vk::PipelineColorBlendStateCreateFlags{},
      false,
      vk::LogicOp::eNoOp,
      1,
      &color_blend_attachment_state,
      {0.0F, 0.0F, 0.0F, 0.0F}
  };

  vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state{
      vk::PipelineMultisampleStateCreateFlags{},
      vk::SampleCountFlagBits::e1,
      false,
      0.0F,
      nullptr,
      false,
      false,
      nullptr
  };

  vk::raii::DescriptorSetLayout layout{device, vk::DescriptorSetLayoutCreateInfo{
      vk::DescriptorSetLayoutCreateFlags{}
  }};
  std::vector<vk::DescriptorSetLayout> descriptor_set_layouts{};
  std::vector<vk::PushConstantRange> push_constant_ranges{
      vk::PushConstantRange{vk::ShaderStageFlagBits::eFragment, 0, sizeof(glm::vec2)}
  };
  vk::raii::PipelineLayout pipeline_layout{device, vk::PipelineLayoutCreateInfo{
      vk::PipelineLayoutCreateFlags{},
      descriptor_set_layouts,
      push_constant_ranges
  }};

  vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{
      0,
      1,
      &surface_format.format,
      vk::Format::eUndefined,
      vk::Format::eUndefined,
      nullptr
  };

  vk::raii::Pipeline graphics_pipeline{device, nullptr, vk::GraphicsPipelineCreateInfo{
      vk::PipelineCreateFlags{},
      shader_stages,
      &vertex_input_state,
      &pipeline_input_assembly_state,
      &pipeline_tessellation_state,
      &pipeline_viewport_state,
      &pipeline_rasterization_state,
      &pipeline_multisample_state,
      nullptr,
      &pipeline_color_blend_state,
      nullptr,
      *pipeline_layout,
      nullptr,
      0,
      nullptr,
      0,
      &pipeline_rendering_create_info
  }};

  std::vector<glm::vec2> push_constants = {glm::vec2{image_size.width, image_size.height}};
  bool should_app_close = false;
  while (!should_app_close) {
    auto start_frame_time = std::chrono::high_resolution_clock::now();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        should_app_close = true;
      }
    }

    // acquire a swapchain image
    auto image_result =
        swapchain_khr.acquireNextImage(target_frame_time_ns, *image_acquired_semaphore, nullptr);
    if (image_result.first == vk::Result::eTimeout) {
      Logging::Log(Logging::Level::kWarning, "Acquire next image timed out.");
      continue;
    }
    uint32_t image_index = image_result.second;

    vk::RenderingAttachmentInfo color_attachment_info{
        *swapchain_image_views[image_index],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ResolveModeFlagBits::eNone,
        *swapchain_image_views[image_index],
        vk::ImageLayout::ePresentSrcKHR,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 1.0F}},
        nullptr
    };

    auto result = device.waitForFences(*image_presented_fence, true, target_frame_time_ns);
    if (result == vk::Result::eTimeout) {
      Logging::Log(Logging::Level::kWarning, "Waiting for image presented fence timed out.");
      continue;
    }
    device.resetFences(*image_presented_fence);
    command_buffers[0].begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    // transition image to a format that can be rendered to
    {
      vk::ImageMemoryBarrier2 image_memory_barrier
          {vk::PipelineStageFlagBits2::eTopOfPipe, {},
           vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
           vk::ImageLayout::eUndefined,
           vk::ImageLayout::eColorAttachmentOptimal,
           queue_info.family_index,
           queue_info.family_index,
           swapchain_images[image_index],
           vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
           nullptr};
      command_buffers[0].pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                             nullptr,
                                                             nullptr,
                                                             image_memory_barrier,
                                                             nullptr});
    }

    // begin rendering
    command_buffers[0].beginRendering(vk::RenderingInfo{
        vk::RenderingFlags{},
        vk::Rect2D{{0, 0}, image_size},
        1,
        0,
        color_attachment_info,
        nullptr, nullptr, nullptr
    });

    // bind drawing data
    command_buffers[0].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);
    command_buffers[0].pushConstants<glm::vec2>(*pipeline_layout,
                                                vk::ShaderStageFlagBits::eFragment,
                                                0,
                                                push_constants);

    // register draw commands
    {
      vk::ClearAttachment clear_attachment{
          vk::ImageAspectFlagBits::eColor,
          0,
          vk::ClearColorValue{std::array{1.0F, 0.0F, 0.0F, 1.0F}}
      };
      vk::ClearRect clear_rect{
          vk::Rect2D{{0, 0}, image_size},
          0,
          1
      };
      command_buffers[0].clearAttachments(clear_attachment, clear_rect);
    }

    command_buffers[0].draw(6, 1, 0, 0);

    // end rendering
    command_buffers[0].endRendering();

    // transition image to a format that can be presented
    {
      vk::ImageMemoryBarrier2 image_memory_barrier
          {vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
           vk::PipelineStageFlagBits2::eBottomOfPipe, {},
           vk::ImageLayout::eColorAttachmentOptimal,
           vk::ImageLayout::ePresentSrcKHR,
           queue_info.family_index,
           queue_info.family_index,
           swapchain_images[image_index],
           vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
           nullptr};
      command_buffers[0].pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                             nullptr,
                                                             nullptr,
                                                             image_memory_barrier,
                                                             nullptr});
    }

    command_buffers[0].end();

    // submit command buffer
    {
      vk::SemaphoreSubmitInfo wait_semaphore_submit_info
          {*image_acquired_semaphore, 1,
           vk::PipelineStageFlagBits2::eTopOfPipe, 0, nullptr};
      vk::SemaphoreSubmitInfo signal_semaphore_submit_info
          {*rendering_complete_semaphore, 1,
           vk::PipelineStageFlagBits2::eBottomOfPipe, 0, nullptr};
      vk::CommandBufferSubmitInfo command_buffer_submit_info{*command_buffers[0], 0, nullptr};
      graphics_queue.submit2(vk::SubmitInfo2{
          vk::SubmitFlags{},
          wait_semaphore_submit_info,
          command_buffer_submit_info,
          signal_semaphore_submit_info,
          nullptr
      }, *image_presented_fence);
    }

    // present the swapchain image
    result =
        graphics_queue.presentKHR(vk::PresentInfoKHR{*rendering_complete_semaphore, *swapchain_khr, image_index,
                                                     nullptr});
    if (result != vk::Result::eSuccess) {
      Logging::Log(Logging::Level::kWarning, "Presenting swapchain image failed.");
      break;
    }

    auto end_frame_time = std::chrono::high_resolution_clock::now();
    if (end_frame_time - start_frame_time > target_frame_time) {
      Logging::Log(Logging::Level::kInfo,
                   std::format("Frame time: {} ns.",
                               duration_cast<std::chrono::nanoseconds>(end_frame_time - start_frame_time).count()));
    }
  }

  device.waitIdle();
  SDL_Quit();
  return 0;
}
