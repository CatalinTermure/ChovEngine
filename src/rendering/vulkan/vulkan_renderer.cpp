#include "rendering/vulkan/vulkan_renderer.h"

#include "rendering/mesh.h"
#include "rendering/vulkan/allocator.h"
#include "rendering/vulkan/pipeline_builder.h"
#include "rendering/vulkan/shader.h"
#include "windowing/events.h"

#include <filesystem>
#include <fstream>

#include <absl/log/log.h>
#include <glm/glm.hpp>

namespace chove::rendering::vulkan {
using windowing::WindowExtent;
using rendering::Mesh;

namespace {
constexpr vk::Format kColorFormat = vk::Format::eB8G8R8A8Unorm;
constexpr vk::Format kDepthFormat = vk::Format::eD24UnormS8Uint;

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

vk::PhysicalDevice PickPhysicalDevice(const vk::Instance &instance) {
  std::vector<vk::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();
  if (physical_devices.empty()) {
    throw std::runtime_error("No physical devices found.");
  }
  vk::PhysicalDevice physical_device = physical_devices.front();
  for (const auto &potential_device : physical_devices) {
    vk::PhysicalDeviceProperties properties = potential_device.getProperties();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      LOG(INFO) << "Using discrete GPU: " << properties.deviceName;
      physical_device = potential_device;
      break;
    }
  }
  return physical_device;
}

uint32_t GetGraphicsQueueFamilyIndex(vk::SurfaceKHR &surface, const vk::PhysicalDevice &physical_device) {
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
  return graphics_queue_family_index;
}

vk::Device CreateDevice(const vk::PhysicalDevice &physical_device, uint32_t graphics_queue_family_index) {
  std::vector<float> queue_priorities = {1.0F};
  vk::DeviceQueueCreateInfo graphics_queue_create_info{
      vk::DeviceQueueCreateFlags{},
      graphics_queue_family_index,
      queue_priorities
  };

  std::vector<const char *> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  std::vector<const char *> optional_device_extensions = {VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME};
  std::vector<vk::ExtensionProperties> supported_extensions = physical_device.enumerateDeviceExtensionProperties();

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

  vk::Device device = physical_device.createDevice(vk::DeviceCreateInfo{
      vk::DeviceCreateFlags{},
      graphics_queue_create_info,
      nullptr,
      device_extensions,
      nullptr
  });
  return device;
}

vk::RenderPass CreateRenderPass(const vk::Device &device) {
  vk::AttachmentDescription color_attachment{
      vk::AttachmentDescriptionFlags{},
      kColorFormat,
      vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eStore,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageLayout::ePresentSrcKHR
  };

  vk::AttachmentReference color_attachment_ref{
      0,
      vk::ImageLayout::eColorAttachmentOptimal
  };

  vk::AttachmentDescription depth_attachment{
      vk::AttachmentDescriptionFlags{},
      kDepthFormat,
      vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eDontCare,
      vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eDepthStencilAttachmentOptimal,
      vk::ImageLayout::eDepthStencilAttachmentOptimal
  };

  vk::AttachmentReference depth_attachment_ref{
      1,
      vk::ImageLayout::eDepthStencilAttachmentOptimal
  };

  std::vector<vk::AttachmentDescription> attachments = {color_attachment, depth_attachment};

  vk::SubpassDescription subpass{
      vk::SubpassDescriptionFlags{},
      vk::PipelineBindPoint::eGraphics,
      nullptr,
      color_attachment_ref,
      nullptr,
      &depth_attachment_ref,
      nullptr
  };

  vk::RenderPass render_pass = device.createRenderPass(vk::RenderPassCreateInfo{
      vk::RenderPassCreateFlags{},
      attachments,
      subpass,
      nullptr
  });
  return render_pass;
}

vk::SwapchainKHR CreateSwapchain(const windowing::Window &window,
                                 const vk::SurfaceKHR &surface,
                                 const vk::PhysicalDevice &physical_device,
                                 const uint32_t graphics_queue_family_index,
                                 const vk::Device &device) {
  const WindowExtent window_extent = window.extent();
  uint32_t swapchain_width = window_extent.width;
  uint32_t swapchain_height = window_extent.height;
  vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
  if (surface_capabilities.currentExtent.width != UINT32_MAX) {
    swapchain_width = surface_capabilities.currentExtent.width;
    swapchain_height = surface_capabilities.currentExtent.height;
  }

  std::vector<vk::SurfaceFormatKHR> formats = physical_device.getSurfaceFormatsKHR(surface);
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

  vk::SwapchainKHR swapchain = device.createSwapchainKHR(vk::SwapchainCreateInfoKHR{
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
  vk::ImageCreateInfo image_create_info{
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
  vk::Image depth_buffer = allocator.AllocateImage(image_create_info, allocation_create_info);
  return depth_buffer;
}
} // namespace

VulkanRenderer VulkanRenderer::Create(windowing::Window &window) {
  const WindowExtent window_extent = window.extent();

  vk::Instance instance = CreateInstance();
  vk::SurfaceKHR surface = window.CreateSurface(instance);
  const vk::PhysicalDevice physical_device = PickPhysicalDevice(instance);
  const uint32_t graphics_queue_family_index = GetGraphicsQueueFamilyIndex(surface, physical_device);
  const vk::Device device = CreateDevice(physical_device, graphics_queue_family_index);
  vk::CommandPool graphics_command_pool = device.createCommandPool(vk::CommandPoolCreateInfo{
      vk::CommandPoolCreateFlags{},
      graphics_queue_family_index
  });
  auto allocator = Allocator::Create(instance, physical_device, device);

  const vk::RenderPass render_pass = CreateRenderPass(device);

  vk::SwapchainKHR swapchain = CreateSwapchain(window, surface, physical_device, graphics_queue_family_index, device);
  std::array<RenderAttachments, kMaxFramesInFlight>
      render_attachments = CreateFramebuffers(window_extent, device, allocator, render_pass, swapchain);

  return VulkanRenderer{&window, instance, surface, physical_device, device, graphics_queue_family_index,
                        graphics_command_pool, render_pass, swapchain, std::move(allocator), render_attachments};
}

std::array<VulkanRenderer::RenderAttachments, VulkanRenderer::kMaxFramesInFlight> VulkanRenderer::CreateFramebuffers(
    const WindowExtent &window_extent,
    const vk::Device &device,
    Allocator &allocator,
    const vk::RenderPass &render_pass,
    vk::SwapchainKHR &swapchain) {
  std::vector<vk::Image> swapchain_images = device.getSwapchainImagesKHR(swapchain);
  std::vector<vk::ImageView> swapchain_image_views;
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
    std::array<vk::ImageView, 2> attachments = {swapchain_image_views.at(i), depth_buffer_views.at(i)};
    framebuffers.at(i) = device.createFramebuffer(vk::FramebufferCreateInfo{
        vk::FramebufferCreateFlags{},
        render_pass,
        attachments,
        window_extent.width,
        window_extent.height,
        1
    });
  }

  std::array<RenderAttachments, kMaxFramesInFlight> render_attachments;
  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    render_attachments.at(i) = RenderAttachments{
        depth_buffers.at(i),
        swapchain_image_views.at(i),
        depth_buffer_views.at(i),
        framebuffers.at(i)
    };
  }
  return render_attachments;
}

void VulkanRenderer::Render() {
  // Check if resizing is necessary
  windowing::Event *event = window_->GetEvent();
  if (event != nullptr && event->type() == windowing::EventType::kWindowResize) {
    for (const auto &render_attachment : render_attachments_) {
      context_.device.destroyImageView(render_attachment.color_attachment_view);
      context_.device.destroyImageView(render_attachment.depth_attachment_view);
      allocator_.Deallocate(render_attachment.depth_attachment);
      context_.device.destroyFramebuffer(render_attachment.framebuffer);
    }
    context_.device.destroySwapchainKHR(swapchain_);

    swapchain_ = CreateSwapchain(*window_, surface_, physical_device_, graphics_queue_family_index_, context_.device);
    render_attachments_ = CreateFramebuffers(window_->extent(), context_.device, allocator_, render_pass_, swapchain_);
    window_->HandleEvent(event);
  } else if (event != nullptr) {
    window_->ReturnEvent(event);
  }

  chove::windowing::WindowExtent window_extent = window_->extent();

  const std::vector<vk::CommandBuffer> command_buffers = context_.device.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo{
          graphics_command_pool_,
          vk::CommandBufferLevel::ePrimary,
          1
      });
  vk::CommandBuffer draw_cmd = command_buffers.front();

  uint32_t image_index = 0;
  {
    vk::ResultValue<uint32_t> result = context_.device.acquireNextImage2KHR(vk::AcquireNextImageInfoKHR{
        swapchain_,
        UINT64_MAX,
        VK_NULL_HANDLE, // TODO: semaphore
        VK_NULL_HANDLE, // TODO: fence
        UINT32_MAX
    });
    if (result.result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to acquire next image.");
    }
    image_index = result.value;
  }

  draw_cmd.begin(vk::CommandBufferBeginInfo{
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
      nullptr
  });

  {
    vk::ClearValue clear_color{std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}};
    vk::ClearValue clear_depth{vk::ClearDepthStencilValue{1.0F, 0}};
    std::vector clear_values = {clear_color, clear_depth};
    draw_cmd.beginRenderPass(vk::RenderPassBeginInfo{
        render_pass_,
        render_attachments_.at(image_index).framebuffer,
        vk::Rect2D{{0, 0}, {window_extent.width, window_extent.height}},
        clear_values
    }, vk::SubpassContents::eInline);

    draw_cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines_.front());
    draw_cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                pipeline_layouts_.front(),
                                0,
                                descriptor_sets_.front(),
                                nullptr);

    for (const auto &obj : scene_->objects()) {
      glm::mat4 model_matrix = obj.transform->GetMatrix();
      draw_cmd.pushConstants(pipeline_layouts_.front(),
                             vk::ShaderStageFlagBits::eVertex,
                             0,
                             sizeof(glm::mat4),
                             &model_matrix);
      // TODO: bind vertex buffer
      draw_cmd.draw(obj.mesh->vertices.size(), 1, 0, 0);
    }

    draw_cmd.endRenderPass();
  }

  draw_cmd.end();

  context_.device.freeCommandBuffers(graphics_command_pool_, command_buffers);
  context_.device.resetCommandPool(graphics_command_pool_, vk::CommandPoolResetFlagBits::eReleaseResources);
}

void VulkanRenderer::SetupScene(const objects::Scene &scene) {
  Shader vertex_shader{"shaders/vulkan/vulkan_shader.vert.spv", context_.device};
  vertex_shader.AddPushConstantRanges({vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)}});
  vertex_shader.AddDescriptorSetLayout({vk::DescriptorSetLayoutBinding{
      0,
      vk::DescriptorType::eUniformBuffer,
      1,
      vk::ShaderStageFlagBits::eVertex
  }});
  vk::VertexInputBindingDescription vertex_binding_description{
      0,
      static_cast<uint32_t>(sizeof(Mesh::Vertex)),
      vk::VertexInputRate::eVertex
  };
  vk::VertexInputAttributeDescription position_attribute_description{
      0,
      0,
      vk::Format::eR32G32B32Sfloat,
      static_cast<uint32_t>(offsetof(Mesh::Vertex, position))
  };
  vk::VertexInputAttributeDescription normal_attribute_description{
      1,
      0,
      vk::Format::eR32G32B32Sfloat,
      static_cast<uint32_t>(offsetof(Mesh::Vertex, normal))
  };
  Shader fragment_shader{"shaders/vulkan/vulkan_shader.frag.spv", context_.device};
  PipelineBuilder pipeline_builder{context_.device};
  auto [pipeline, layout] = pipeline_builder.SetVertexShader(vertex_shader)
      .SetFragmentShader(fragment_shader)
      .SetInputTopology(vk::PrimitiveTopology::eTriangleList)
      .AddInputBufferDescription(vertex_binding_description,
                                 {position_attribute_description, normal_attribute_description})
      .SetViewport(vk::Viewport{0.0F,
                                0.0F,
                                static_cast<float>(window_->extent().width),
                                static_cast<float>(window_->extent().height),
                                0.0F,
                                1.0F})
      .SetScissor(vk::Rect2D{{0, 0}, vk::Extent2D{window_->extent().width, window_->extent().height}})
      .SetFillMode(vk::PolygonMode::eFill)
      .SetColorBlendEnable(false)
      .SetDepthTestEnable(true)
      .build(render_pass_, 0);
  pipelines_.push_back(pipeline);
  pipeline_layouts_.push_back(layout);

  scene_ = &scene;

  vk::DescriptorPoolSize pool_size{vk::DescriptorType::eUniformBuffer, 1};
  descriptor_pool_ = context_.device.createDescriptorPool(vk::DescriptorPoolCreateInfo{
      vk::DescriptorPoolCreateFlags{},
      1,
      pool_size
  });

  vk::DescriptorSetLayout descriptor_set_layout = vertex_shader.descriptor_set_layouts().front();
  descriptor_sets_.push_back(context_.device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
      descriptor_pool_,
      descriptor_set_layout
  }).front());

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

  for (const auto &render_attachment : render_attachments_) {
    context_.device.destroyImageView(render_attachment.color_attachment_view);
    context_.device.destroyImageView(render_attachment.depth_attachment_view);
    context_.device.destroyFramebuffer(render_attachment.framebuffer);
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

VulkanRenderer::VulkanRenderer(windowing::Window *window,
                               vk::Instance instance,
                               vk::SurfaceKHR surface,
                               vk::PhysicalDevice physical_device,
                               vk::Device device,
                               uint32_t graphics_queue_family_index,
                               vk::CommandPool graphics_command_pool,
                               vk::RenderPass render_pass,
                               vk::SwapchainKHR swapchain,
                               Allocator allocator,
                               const std::array<RenderAttachments, kMaxFramesInFlight> &render_attachments)
    : context_(device, instance),
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
}

VulkanRenderer::VulkanRenderer(VulkanRenderer &&other) noexcept {
  *this = std::move(other);
}

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
} // namespace chove::rendering::vulkan
