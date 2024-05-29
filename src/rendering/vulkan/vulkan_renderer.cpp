#include "rendering/vulkan/vulkan_renderer.h"

#include "rendering/mesh.h"
#include "rendering/vulkan/allocator.h"
#include "rendering/vulkan/pipeline_builder.h"
#include "rendering/vulkan/shader.h"
#include "windowing/events.h"
#include "windowing/window.h"

#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

#include <absl/log/log.h>
#include <vulkan/vulkan.hpp>

#include "rendering/vulkan/render_info.h"

namespace chove::rendering::vulkan {

using objects::Transform;
using rendering::Mesh;
using windowing::WindowExtent;

namespace {
constexpr auto kColorFormat = vk::Format::eB8G8R8A8Unorm;
constexpr auto kDepthFormat = vk::Format::eD24UnormS8Uint;

vk::Instance CreateInstance() {
  std::vector<const char *> required_instance_extensions = windowing::Window::GetRequiredVulkanExtensions();
  constexpr vk::ApplicationInfo application_info{
      "Demo app", VK_MAKE_VERSION(1, 0, 0), "ChovEngine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_3
  };
  const vk::InstanceCreateInfo instance_create_info{
      vk::InstanceCreateFlags{}, &application_info, nullptr, required_instance_extensions
  };
  const vk::Instance instance = createInstance(instance_create_info, nullptr);
  return instance;
}

vk::PhysicalDevice PickPhysicalDevice(const vk::Instance &instance) {
  const std::vector<vk::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();
  if (physical_devices.empty()) {
    throw std::runtime_error("No physical devices found.");
  }
  vk::PhysicalDevice physical_device = physical_devices.front();
  for (const auto &potential_device : physical_devices) {
    const vk::PhysicalDeviceProperties properties = potential_device.getProperties();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      LOG(INFO) << "Using discrete GPU: " << properties.deviceName;
      physical_device = potential_device;
      break;
    }
  }
  return physical_device;
}

uint32_t GetGraphicsQueueFamilyIndex(const vk::SurfaceKHR &surface, const vk::PhysicalDevice &physical_device) {
  const std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();
  uint32_t graphics_queue_family_index = -1;
  for (uint32_t family_index = 0; family_index < queue_family_properties.size(); ++family_index) {
    if ((queue_family_properties[family_index].queueFlags & vk::QueueFlagBits::eGraphics) &&
        physical_device.getSurfaceSupportKHR(family_index, surface) != 0) {
      LOG(INFO) << "Found graphics queue family with presentation support at index " << family_index;
      graphics_queue_family_index = family_index;
      break;
    }
  }
  if (graphics_queue_family_index == -1) {
    throw std::runtime_error("No graphics queue family with presentation support found.");
  }
  return graphics_queue_family_index;
}

vk::Device CreateDevice(const vk::PhysicalDevice &physical_device, uint32_t graphics_queue_family_index) {
  std::vector queue_priorities = {1.0F};
  vk::DeviceQueueCreateInfo graphics_queue_create_info{
      vk::DeviceQueueCreateFlags{}, graphics_queue_family_index, queue_priorities
  };

  std::vector device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const std::vector optional_device_extensions = {VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME};
  const std::vector<vk::ExtensionProperties> supported_extensions =
      physical_device.enumerateDeviceExtensionProperties();

  for (const auto &optional_extension : optional_device_extensions) {
    bool found = false;
    for (const auto &supported_extension : supported_extensions) {
      if (std::strcmp(supported_extension.extensionName, optional_extension) == 0) {
        found = true;
        break;
      }
    }
    if (found) {
      device_extensions.push_back(optional_extension);
    }
  }

  vk::PhysicalDeviceFeatures2 required_features{};
  vk::PhysicalDeviceSynchronization2Features synchronization2_features{vk::True};
  required_features.pNext = &synchronization2_features;

  const vk::Device device = physical_device.createDevice(vk::DeviceCreateInfo{
      vk::DeviceCreateFlags{}, graphics_queue_create_info, nullptr, device_extensions, nullptr, &required_features
  });
  return device;
}

vk::RenderPass CreateRenderPass(const vk::Device &device) {
  constexpr vk::AttachmentDescription color_attachment{
      vk::AttachmentDescriptionFlags{},
      kColorFormat,
      vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eStore,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::ePresentSrcKHR
  };

  vk::AttachmentReference color_attachment_ref{0, vk::ImageLayout::eColorAttachmentOptimal};

  constexpr vk::AttachmentDescription depth_attachment{
      vk::AttachmentDescriptionFlags{},
      kDepthFormat,
      vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eDontCare,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eDepthStencilAttachmentOptimal
  };

  constexpr vk::AttachmentReference depth_attachment_ref{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

  const std::vector attachments = {color_attachment, depth_attachment};

  const vk::SubpassDescription subpass{
      vk::SubpassDescriptionFlags{},
      vk::PipelineBindPoint::eGraphics,
      nullptr,
      color_attachment_ref,
      nullptr,
      &depth_attachment_ref,
      nullptr
  };

  const std::vector dependencies = {
      vk::SubpassDependency{
          VK_SUBPASS_EXTERNAL,
          0,
          vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
          vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
          vk::AccessFlagBits::eNone,
          vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
      },
      vk::SubpassDependency{
          0,
          VK_SUBPASS_EXTERNAL,
          vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
          vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
          vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
          vk::AccessFlagBits::eNone,
      }
  };

  const vk::RenderPass render_pass =
      device.createRenderPass(vk::RenderPassCreateInfo{vk::RenderPassCreateFlags{}, attachments, subpass, dependencies}
      );
  return render_pass;
}

vk::SwapchainKHR CreateSwapchain(
    const windowing::Window &window,
    const vk::SurfaceKHR &surface,
    const vk::PhysicalDevice &physical_device,
    const uint32_t graphics_queue_family_index,
    const vk::Device &device
) {
  auto [swapchain_width, swapchain_height] = window.extent();
  const vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
  if (surface_capabilities.currentExtent.width != UINT32_MAX) {
    swapchain_width = surface_capabilities.currentExtent.width;
    swapchain_height = surface_capabilities.currentExtent.height;
  }

  const std::vector<vk::SurfaceFormatKHR> formats = physical_device.getSurfaceFormatsKHR(surface);
  vk::SurfaceFormatKHR surface_format;
  bool found = false;
  for (const auto &format : formats) {
    if (format.format == kColorFormat && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      surface_format = format;
      found = true;
      break;
    }
  }

  if (!found) {
    throw std::runtime_error("No suitable surface format found.");
  }

  const vk::SwapchainKHR swapchain = device.createSwapchainKHR(vk::SwapchainCreateInfoKHR{
      vk::SwapchainCreateFlagsKHR{},
      surface,
      VulkanRenderer::kMaxFramesInFlight,
      surface_format.format,
      surface_format.colorSpace,
      vk::Extent2D{swapchain_width, swapchain_height},
      1,
      vk::ImageUsageFlagBits::eColorAttachment,
      vk::SharingMode::eExclusive,
      graphics_queue_family_index,
      vk::SurfaceTransformFlagBitsKHR::eIdentity,
      vk::CompositeAlphaFlagBitsKHR::eOpaque,
      vk::PresentModeKHR::eFifo,
      VK_TRUE,
      VK_NULL_HANDLE
  });
  return swapchain;
}

vk::Image CreateDepthBuffer(const WindowExtent &window_extent, Allocator &allocator) {
  const vk::ImageCreateInfo image_create_info{
      vk::ImageCreateFlags{},
      vk::ImageType::e2D,
      kDepthFormat,
      vk::Extent3D{window_extent.width, window_extent.height, 1},
      1,
      1,
      vk::SampleCountFlagBits::e1,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
      vk::SharingMode::eExclusive,
      0,
      nullptr,
      vk::ImageLayout::eUndefined
  };
  VmaAllocationCreateInfo allocation_create_info = {};
  allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
  allocation_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  allocation_create_info.priority = 1.0F;
  const vk::Image depth_buffer = allocator.AllocateImage(image_create_info, allocation_create_info);
  return depth_buffer;
}
}  // namespace

VulkanRenderer VulkanRenderer::Create(windowing::Window &window) {
  const WindowExtent window_extent = window.extent();

  const vk::Instance instance = CreateInstance();
  const vk::SurfaceKHR surface = window.CreateSurface(instance);
  const vk::PhysicalDevice physical_device = PickPhysicalDevice(instance);
  const uint32_t graphics_queue_family_index = GetGraphicsQueueFamilyIndex(surface, physical_device);
  const vk::Device device = CreateDevice(physical_device, graphics_queue_family_index);
  const vk::CommandPool graphics_command_pool =
      device.createCommandPool(vk::CommandPoolCreateInfo{vk::CommandPoolCreateFlags{}, graphics_queue_family_index});
  auto allocator = Allocator::Create(instance, physical_device, device);

  const vk::RenderPass render_pass = CreateRenderPass(device);

  const vk::SwapchainKHR swapchain =
      CreateSwapchain(window, surface, physical_device, graphics_queue_family_index, device);
  const std::array<RenderAttachments, kMaxFramesInFlight> render_attachments =
      CreateFramebuffers(window_extent, device, allocator, render_pass, swapchain);

  return VulkanRenderer{
      &window,
      instance,
      surface,
      physical_device,
      device,
      graphics_queue_family_index,
      graphics_command_pool,
      render_pass,
      swapchain,
      std::move(allocator),
      render_attachments
  };
}

std::array<VulkanRenderer::RenderAttachments, VulkanRenderer::kMaxFramesInFlight> VulkanRenderer::CreateFramebuffers(
    const WindowExtent &window_extent,
    const vk::Device &device,
    Allocator &allocator,
    const vk::RenderPass &render_pass,
    const vk::SwapchainKHR &swapchain
) {
  const std::vector<vk::Image> swapchain_images = device.getSwapchainImagesKHR(swapchain);
  std::vector<vk::ImageView> swapchain_image_views;
  swapchain_image_views.reserve(swapchain_images.size());
  for (const auto &image : swapchain_images) {
    swapchain_image_views.push_back(device.createImageView(vk::ImageViewCreateInfo{
        vk::ImageViewCreateFlags{},
        image,
        vk::ImageViewType::e2D,
        kColorFormat,
        vk::ComponentMapping{},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    }));
  }

  std::array<vk::Image, kMaxFramesInFlight> depth_buffers;
  for (auto &depth_buffer : depth_buffers) {
    depth_buffer = CreateDepthBuffer(window_extent, allocator);
  }
  std::vector<vk::ImageView> depth_buffer_views;
  depth_buffer_views.reserve(depth_buffers.size());
  for (const auto &depth_buffer : depth_buffers) {
    depth_buffer_views.push_back(device.createImageView(vk::ImageViewCreateInfo{
        vk::ImageViewCreateFlags{},
        depth_buffer,
        vk::ImageViewType::e2D,
        kDepthFormat,
        vk::ComponentMapping{},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1}
    }));
  }

  std::array<vk::Framebuffer, kMaxFramesInFlight> framebuffers;
  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    std::array attachments = {swapchain_image_views.at(i), depth_buffer_views.at(i)};
    framebuffers.at(i) = device.createFramebuffer(vk::FramebufferCreateInfo{
        vk::FramebufferCreateFlags{}, render_pass, attachments, window_extent.width, window_extent.height, 1
    });
  }

  std::array<RenderAttachments, kMaxFramesInFlight> render_attachments;
  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    render_attachments.at(i) = RenderAttachments{
        depth_buffers.at(i), swapchain_image_views.at(i), depth_buffer_views.at(i), framebuffers.at(i)
    };
  }
  return render_attachments;
}

void VulkanRenderer::Render() {
  // Check if resizing is necessary
  windowing::Event *event = window_->GetEvent();
  if (event != nullptr && event->type() == windowing::EventType::kWindowResize) {
    for (const auto &[depth_attachment, color_attachment_view, depth_attachment_view, framebuffer] :
         render_attachments_) {
      context_.device.destroyImageView(color_attachment_view);
      context_.device.destroyImageView(depth_attachment_view);
      allocator_.Deallocate(depth_attachment);
      context_.device.destroyFramebuffer(framebuffer);
    }
    context_.device.destroySwapchainKHR(swapchain_);

    swapchain_ = CreateSwapchain(*window_, surface_, physical_device_, graphics_queue_family_index_, context_.device);
    render_attachments_ = CreateFramebuffers(window_->extent(), context_.device, allocator_, render_pass_, swapchain_);
    window_->HandleEvent(event);
  }
  else if (event != nullptr) {
    window_->ReturnEvent(event);
  }

  current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;

  const auto [window_width, window_height] = window_->extent();

  {
    const vk::Result result =
        context_.device.waitForFences(synchronization_info_.at(current_frame_).in_flight_fence, vk::True, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
      LOG(INFO) << "Failed to wait for frame in flight at frame " << current_frame_ << ".";
      return;
    }
  }

  context_.device.resetFences(synchronization_info_.at(current_frame_).in_flight_fence);

  command_buffers_.at(current_frame_) = context_.device.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo{graphics_command_pool_, vk::CommandBufferLevel::ePrimary, 1}
  );
  const vk::CommandBuffer draw_cmd = command_buffers_.at(current_frame_).front();

  const uint32_t image_index = [&] {
    const vk::ResultValue<uint32_t> result = context_.device.acquireNextImage2KHR(vk::AcquireNextImageInfoKHR{
        swapchain_,
        std::numeric_limits<uint64_t>::max(),
        synchronization_info_.at(current_frame_).image_available,
        VK_NULL_HANDLE,
        1
    });
    if (result.result != vk::Result::eSuccess) {
      return std::numeric_limits<uint32_t>::max();
    }
    return result.value;
  }();

  if (image_index == std::numeric_limits<uint32_t>::max()) {
    LOG(INFO) << "Skipped frame.";
    return;
  }

  draw_cmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr});

  {
    constexpr vk::ClearValue clear_color{std::array{0.0F, 0.0F, 0.0F, 1.0F}};
    constexpr vk::ClearValue clear_depth{vk::ClearDepthStencilValue{1.0F, 0}};
    std::vector clear_values = {clear_color, clear_depth};
    draw_cmd.beginRenderPass(
        vk::RenderPassBeginInfo{
            render_pass_,
            render_attachments_.at(image_index).framebuffer,
            vk::Rect2D{{0, 0}, {window_width, window_height}},
            clear_values
        },
        vk::SubpassContents::eInline
    );

    draw_cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines_.front());

    const glm::mat4 camera_matrix = scene_->camera().GetProjectionMatrix() * scene_->camera().GetViewMatrix();

    for (const auto &&[_, mesh, render_info] : scene_->GetAllObjectsWith<Mesh *, RenderInfo>().each()) {
      const auto index_count = mesh->indices.size();
      memcpy(render_info.vertex_buffer_memory, mesh->vertices.data(), mesh->vertices.size() * sizeof(Mesh::Vertex));
      memcpy(render_info.index_buffer_memory, mesh->indices.data(), index_count * sizeof(uint32_t));
      draw_cmd.bindVertexBuffers(
          0,
          {render_info.vertex_buffer, render_info.vertex_buffer},
          {offsetof(Mesh::Vertex, position), offsetof(Mesh::Vertex, normal)}
      );
      draw_cmd.bindIndexBuffer(render_info.index_buffer, vk::DeviceSize{0}, vk::IndexType::eUint32);
      const glm::mat4 model_view_projection = camera_matrix * render_info.model;
      draw_cmd.pushConstants(
          pipeline_layouts_.front(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &model_view_projection
      );
      draw_cmd.drawIndexed(index_count, 1, 0, 0, 0);
    }

    draw_cmd.endRenderPass();
  }

  draw_cmd.end();

  {
    vk::CommandBufferSubmitInfo draw_submit_info{draw_cmd, 1};
    const vk::SemaphoreSubmitInfo wait_for_image_acquisition{
        synchronization_info_.at(current_frame_).image_available, 1, vk::PipelineStageFlagBits2::eColorAttachmentOutput
    };
    std::array wait_semaphores = {wait_for_image_acquisition};

    vk::SemaphoreSubmitInfo signal_semaphore_info{
        synchronization_info_.at(current_frame_).render_finished, 1, vk::PipelineStageFlagBits2::eAllGraphics
    };
    graphics_queue_.submit2(
        vk::SubmitInfo2{vk::SubmitFlags{}, wait_semaphores, draw_submit_info, signal_semaphore_info},
        synchronization_info_.at(current_frame_).in_flight_fence
    );
  }

  const vk::Result result = graphics_queue_.presentKHR(
      vk::PresentInfoKHR{synchronization_info_.at(current_frame_).render_finished, swapchain_, image_index, nullptr}
  );

  if (result != vk::Result::eSuccess) {
    LOG(INFO) << "Failed to present frame.";
  }
}

void VulkanRenderer::SetupScene(objects::Scene &scene) {
  Shader vertex_shader{"shaders/vulkan/vulkan_shader.vert.spv", context_.device};
  vertex_shader.AddPushConstantRanges({vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)}});
  // vertex_shader.AddDescriptorSetLayout(
  //     {vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}}
  // );
  constexpr vk::VertexInputBindingDescription vertex_binding_description{
      0, sizeof(Mesh::Vertex), vk::VertexInputRate::eVertex
  };
  const vk::VertexInputAttributeDescription position_attribute_description{
      0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Mesh::Vertex, position))
  };
  const vk::VertexInputAttributeDescription normal_attribute_description{
      1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Mesh::Vertex, normal))
  };
  Shader fragment_shader{"shaders/vulkan/vulkan_shader.frag.spv", context_.device};
  PipelineBuilder pipeline_builder{context_.device};
  auto [pipeline, layout] =
      pipeline_builder.SetVertexShader(vertex_shader)
          .SetFragmentShader(fragment_shader)
          .SetInputTopology(vk::PrimitiveTopology::eTriangleList)
          .AddInputBufferDescription(
              vertex_binding_description, {position_attribute_description, normal_attribute_description}
          )
          .SetViewport(vk::Viewport{
              0.0F,
              0.0F,
              static_cast<float>(window_->extent().width),
              static_cast<float>(window_->extent().height),
              0.0F,
              1.0F
          })
          .SetScissor(vk::Rect2D{{0, 0}, vk::Extent2D{window_->extent().width, window_->extent().height}})
          .SetFillMode(vk::PolygonMode::eFill)
          .SetColorBlendEnable(false)
          .SetDepthTestEnable(true)
          .build(render_pass_, 0);
  pipelines_.push_back(pipeline);
  pipeline_layouts_.push_back(layout);

  scene_ = &scene;

  // vk::DescriptorPoolSize pool_size{vk::DescriptorType::eUniformBuffer, 1};
  // descriptor_pool_ =
  //     context_.device.createDescriptorPool(vk::DescriptorPoolCreateInfo{vk::DescriptorPoolCreateFlags{}, 1,
  //     pool_size});
  //
  // vk::DescriptorSetLayout descriptor_set_layout = vertex_shader.descriptor_set_layouts().front();
  // descriptor_sets_.push_back(
  //     context_.device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{descriptor_pool_, descriptor_set_layout})
  //         .front()
  // );

  for (auto &&[entity, transform, mesh] : scene_->GetAllObjectsWith<Transform, Mesh *>().each()) {
    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocation_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocation_create_info.priority = 1.0F;

    RenderInfo render_info;
    render_info.vertex_buffer = allocator_.AllocateBuffer(
        vk::BufferCreateInfo{
            vk::BufferCreateFlags{},
            mesh->vertices.size() * sizeof(Mesh::Vertex),
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::SharingMode::eExclusive,
            graphics_queue_family_index_
        },
        allocation_create_info
    );
    render_info.vertex_buffer_memory = allocator_.GetMappedMemory(render_info.vertex_buffer);
    render_info.index_buffer = allocator_.AllocateBuffer(
        vk::BufferCreateInfo{
            vk::BufferCreateFlags{},
            mesh->indices.size() * sizeof(uint32_t),
            vk::BufferUsageFlagBits::eIndexBuffer,
            vk::SharingMode::eExclusive,
            graphics_queue_family_index_
        },
        allocation_create_info
    );
    render_info.index_buffer_memory = allocator_.GetMappedMemory(render_info.index_buffer);
    render_info.model = transform.GetMatrix();

    scene_->AddComponent<RenderInfo>(entity, std::move(render_info));
  }

  shaders_.push_back(std::move(vertex_shader));
  shaders_.push_back(std::move(fragment_shader));
}

VulkanRenderer::~VulkanRenderer() {
  if (!context_.device) {
    return;
  }
  context_.device.waitIdle();

  for (const auto &pipeline : pipelines_) {
    context_.device.destroyPipeline(pipeline);
  }
  for (const auto &layout : pipeline_layouts_) {
    context_.device.destroyPipelineLayout(layout);
  }

  for (const auto &[depth_attachment, color_attachment_view, depth_attachment_view, framebuffer] :
       render_attachments_) {
    context_.device.destroyImageView(color_attachment_view);
    context_.device.destroyImageView(depth_attachment_view);
    context_.device.destroyFramebuffer(framebuffer);
  }

  if (swapchain_) {
    context_.device.destroySwapchainKHR(swapchain_);
  }

  if (render_pass_) {
    context_.device.destroyRenderPass(render_pass_);
  }
  if (graphics_command_pool_) {
    context_.device.destroyCommandPool(graphics_command_pool_);
  }
  if (descriptor_pool_) {
    context_.device.destroyDescriptorPool(descriptor_pool_);
  }
  if (surface_) {
    context_.instance.destroySurfaceKHR(surface_);
  }
}

VulkanRenderer::VulkanRenderer(
    windowing::Window *window,
    const vk::Instance instance,
    const vk::SurfaceKHR surface,
    const vk::PhysicalDevice physical_device,
    const vk::Device device,
    const uint32_t graphics_queue_family_index,
    const vk::CommandPool graphics_command_pool,
    const vk::RenderPass render_pass,
    const vk::SwapchainKHR swapchain,
    Allocator allocator,
    const std::array<RenderAttachments, kMaxFramesInFlight> &render_attachments
) :
    context_(device, instance),
    surface_(surface),
    physical_device_(physical_device),
    allocator_(std::move(allocator)),
    window_(window),
    swapchain_(swapchain),
    render_attachments_(render_attachments),
    graphics_queue_family_index_(graphics_queue_family_index),
    graphics_command_pool_(graphics_command_pool),
    render_pass_(render_pass) {
  graphics_queue_ = context_.device.getQueue(graphics_queue_family_index, 0);
  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    synchronization_info_.at(i) = SynchronizationInfo{
        allocator_.CreateSemaphore(),
        allocator_.CreateSemaphore(),
        allocator_.CreateFence(vk::FenceCreateFlagBits::eSignaled)
    };
  }
}

VulkanRenderer::VulkanRenderer(VulkanRenderer &&other) noexcept { *this = std::move(other); }

VulkanRenderer &VulkanRenderer::operator=(VulkanRenderer &&other) noexcept {
  if (this != &other) {
    window_ = other.window_;
    surface_ = other.surface_;
    physical_device_ = other.physical_device_;
    graphics_queue_ = other.graphics_queue_;
    graphics_queue_family_index_ = other.graphics_queue_family_index_;
    graphics_command_pool_ = other.graphics_command_pool_;
    render_pass_ = other.render_pass_;
    swapchain_ = other.swapchain_;
    allocator_ = std::move(other.allocator_);
    render_attachments_ = other.render_attachments_;
    shaders_ = std::move(other.shaders_);
    pipelines_ = std::move(other.pipelines_);
    pipeline_layouts_ = std::move(other.pipeline_layouts_);
    context_ = std::move(other.context_);
    descriptor_pool_ = other.descriptor_pool_;
    descriptor_sets_ = std::move(other.descriptor_sets_);
    synchronization_info_ = other.synchronization_info_;
    scene_ = other.scene_;

    other.window_ = nullptr;
    other.surface_ = VK_NULL_HANDLE;
    other.physical_device_ = VK_NULL_HANDLE;
    other.graphics_queue_ = VK_NULL_HANDLE;
    other.graphics_queue_family_index_ = -1;
    other.graphics_command_pool_ = VK_NULL_HANDLE;
    other.render_pass_ = VK_NULL_HANDLE;
    other.swapchain_ = VK_NULL_HANDLE;
    other.render_attachments_ = {};
    other.descriptor_pool_ = VK_NULL_HANDLE;
    other.scene_ = nullptr;
  }
  return *this;
}
}  // namespace chove::rendering::vulkan
