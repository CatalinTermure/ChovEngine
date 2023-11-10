#include "rendering/vulkan/vulkan_renderer.h"
#include "rendering/vulkan/shader.h"
#include "rendering/vulkan/pipeline_builder.h"

#include <absl/status/statusor.h>
#include <absl/log/log.h>

namespace chove::rendering::vulkan {
namespace {

constexpr int target_frame_rate = 60;
constexpr long long target_frame_time_ns = 1'000'000'000 / target_frame_rate;

vk::raii::CommandPool CreateCommandPool(uint32_t graphics_queue_index, const vk::raii::Device &device) {
  vk::raii::CommandPool command_pool =
      device.createCommandPool({vk::CommandPoolCreateFlags{},
                                graphics_queue_index});
  return command_pool;
}
std::vector<vk::raii::CommandBuffer> AllocateCommandBuffers(const vk::raii::Device &device,
                                                            const vk::raii::CommandPool &command_pool) {
  auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo{*command_pool, vk::CommandBufferLevel::ePrimary, 1};
  std::vector<vk::raii::CommandBuffer> command_buffers = device.allocateCommandBuffers(command_buffer_allocate_info);
  return command_buffers;
}

}

void VulkanRenderer::Render(const objects::Scene &scene) {
  std::vector<vk::Buffer> vertex_buffers;
  for (const auto &buffer : vertex_buffers_) {
    void *buffer_host_memory = buffer.memory.mapMemory(buffer.offset, buffer.size, vk::MemoryMapFlags{});

    memcpy(buffer_host_memory, buffer.mesh->vertices.data(), buffer.size);

    buffer.memory.unmapMemory();

    vertex_buffers.push_back(*buffer.buffer);
  }

  glm::mat4 view_matrix = scene.camera().GetTransformMatrix();

  // acquire a swapchain image
  auto [image_result, next_image] =
      swapchain_.AcquireNextImage(target_frame_time_ns, *image_acquired_semaphore_, nullptr);
  if (image_result == vk::Result::eTimeout) {
    return;
  }

  vk::RenderingAttachmentInfo color_attachment_info{
      next_image.view,
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ResolveModeFlagBits::eNone,
      next_image.view,
      vk::ImageLayout::ePresentSrcKHR,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eStore,
      vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 0.0F}},
      nullptr
  };

  auto result = context_.device().waitForFences(*image_presented_fence_, true, target_frame_time_ns);
  if (result == vk::Result::eTimeout) {
    LOG(WARNING) << "Previous frame did not present in time.";
    return;
  }
  context_.device().resetFences(*image_presented_fence_);
  
  command_pool_.reset(vk::CommandPoolResetFlags{});
  command_buffers_[0].begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

  // transition image to a format that can be rendered to
  {
    vk::ImageMemoryBarrier2 image_memory_barrier
        {vk::PipelineStageFlagBits2::eTopOfPipe, {},
         vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
         vk::ImageLayout::eUndefined,
         vk::ImageLayout::eColorAttachmentOptimal,
         context_.graphics_queue().family_index,
         context_.graphics_queue().family_index,
         next_image.image,
         vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
         nullptr};
    command_buffers_[0].pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                            nullptr,
                                                            nullptr,
                                                            image_memory_barrier,
                                                            nullptr});
  }

  // begin rendering
  command_buffers_[0].beginRendering(vk::RenderingInfo{
      vk::RenderingFlags{},
      vk::Rect2D{{0, 0}, swapchain_.image_size()},
      1,
      0,
      color_attachment_info,
      nullptr, nullptr, nullptr
  });

  // bind drawing data
  command_buffers_[0].bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelines_[0]);
  command_buffers_[0].pushConstants<glm::mat4>(*pipeline_layouts_[0],
                                               vk::ShaderStageFlagBits::eVertex,
                                               0,
                                               view_matrix);

  // register draw commands
  {
    vk::ClearAttachment clear_attachment{
        vk::ImageAspectFlagBits::eColor,
        0,
        vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 0.0F}}
    };
    vk::ClearRect clear_rect{
        vk::Rect2D{{0, 0}, swapchain_.image_size()},
        0,
        1
    };
    command_buffers_[0].clearAttachments(clear_attachment, clear_rect);
  }
  for (const auto &vertex_buffer : vertex_buffers_) {
    command_buffers_[0].bindVertexBuffers(0, *vertex_buffer.buffer, vertex_buffer.offset);
    command_buffers_[0].draw(vertex_buffer.mesh->vertices.size(), 1, 0, 0);
  }

  // end rendering
  command_buffers_[0].endRendering();

  // transition image to a format that can be presented
  {
    vk::ImageMemoryBarrier2 image_memory_barrier
        {vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
         vk::PipelineStageFlagBits2::eBottomOfPipe, {},
         vk::ImageLayout::eColorAttachmentOptimal,
         vk::ImageLayout::ePresentSrcKHR,
         context_.graphics_queue().family_index,
         context_.graphics_queue().family_index,
         next_image.image,
         vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
         nullptr};
    command_buffers_[0].pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                            nullptr,
                                                            nullptr,
                                                            image_memory_barrier,
                                                            nullptr});
  }

  command_buffers_[0].end();

  // submit command buffer
  {
    vk::SemaphoreSubmitInfo wait_semaphore_submit_info
        {*image_acquired_semaphore_, 1,
         vk::PipelineStageFlagBits2::eTopOfPipe, 0, nullptr};
    vk::SemaphoreSubmitInfo signal_semaphore_submit_info
        {*rendering_complete_semaphore_, 1,
         vk::PipelineStageFlagBits2::eBottomOfPipe, 0, nullptr};
    vk::CommandBufferSubmitInfo command_buffer_submit_info{*command_buffers_[0], 0, nullptr};
    context_.graphics_queue().queue.submit2(vk::SubmitInfo2{
        vk::SubmitFlags{},
        wait_semaphore_submit_info,
        command_buffer_submit_info,
        signal_semaphore_submit_info,
        nullptr
    }, *image_presented_fence_);
  }

  // present the swapchain image
  {
    const vk::SwapchainKHR swapchain_khr = swapchain_.swapchain_khr();
    result = context_.graphics_queue().queue.presentKHR(vk::PresentInfoKHR{*rendering_complete_semaphore_,
                                                                           swapchain_khr,
                                                                           next_image.index,
                                                                           nullptr});
    if (result != vk::Result::eSuccess) {
      LOG(WARNING) << "Presenting swapchain image failed.";
      return;
    }
  }
}

VulkanRenderer::VulkanRenderer(Context context, Swapchain swapchain) :
    context_(std::move(context)),
    swapchain_(std::move(swapchain)),
    image_acquired_semaphore_(context_.device(),
                              vk::SemaphoreCreateInfo{}),
    rendering_complete_semaphore_(context_.device(),
                                  vk::SemaphoreCreateInfo{}),
    image_presented_fence_(context_.device(),
                           vk::FenceCreateInfo{
                               vk::FenceCreateFlagBits::eSignaled}),
    command_pool_(CreateCommandPool(context_.graphics_queue().family_index, context_.device())) {
  command_buffers_ = AllocateCommandBuffers(context_.device(), command_pool_);
}

absl::StatusOr<VulkanRenderer> VulkanRenderer::Create(Window &window) {
  absl::StatusOr<Context> context = Context::CreateContext(window);
  if (!context.ok()) {
    return context.status();
  }

  absl::StatusOr<Swapchain> swapchain = Swapchain::CreateSwapchain(*context, window, 3);
  if (!swapchain.ok()) {
    return swapchain.status();
  }

  return VulkanRenderer{*std::move(context), *std::move(swapchain)};
}

void VulkanRenderer::SetupScene(const objects::Scene &scene) {
  // Create buffers
  for (const auto &object : scene.objects()) {
    vk::raii::Buffer
        vertex_buffer = object.mesh->CreateVertexBuffer(context_.device(), {context_.graphics_queue().family_index});

    vk::MemoryRequirements buffer_memory_requirements = vertex_buffer.getMemoryRequirements();
    vk::MemoryPropertyFlags desired_memory_properties{buffer_memory_requirements.memoryTypeBits & 0x7};

    uint32_t memory_type_index;
    vk::PhysicalDeviceMemoryProperties available_memory_properties = context_.physical_device().getMemoryProperties();
    for (uint32_t i = 0; i < available_memory_properties.memoryTypeCount; i++) {
      if ((available_memory_properties.memoryTypes[i].propertyFlags & desired_memory_properties)
          == desired_memory_properties) {
        memory_type_index = i;
        break;
      }
    }

    vk::raii::DeviceMemory vertex_buffer_memory = context_.device().allocateMemory(vk::MemoryAllocateInfo{
        buffer_memory_requirements.size,
        memory_type_index
    });

    vertex_buffer.bindMemory(*vertex_buffer_memory, 0);

    vertex_buffers_.push_back({object.mesh, std::move(vertex_buffer), std::move(vertex_buffer_memory), 0,
                               buffer_memory_requirements.size});
  }

  // Create pipelines
  {
    vk::VertexInputBindingDescription vertex_input_binding_description{
        0,
        sizeof(Mesh::Vertex),
        vk::VertexInputRate::eVertex
    };
    vk::VertexInputAttributeDescription vertex_input_position_description{
        0,
        0,
        vk::Format::eR32G32B32A32Sfloat,
        static_cast<uint32_t>(offsetof(Mesh::Vertex, position))
    };
    vk::VertexInputAttributeDescription vertex_input_color_description{
        1,
        0,
        vk::Format::eR32G32B32A32Sfloat,
        static_cast<uint32_t>(offsetof(Mesh::Vertex, color))
    };

    std::vector<vk::VertexInputAttributeDescription>
        vertex_input_attributes{vertex_input_position_description, vertex_input_color_description};
    vk::PipelineVertexInputStateCreateInfo vertex_input_state{
        vk::PipelineVertexInputStateCreateFlags{},
        vertex_input_binding_description,
        vertex_input_attributes
    };
    vk::Viewport viewport{
        0.0F,
        0.0F,
        static_cast<float>(swapchain_.image_size().width),
        static_cast<float>(swapchain_.image_size().height),
        0.1F,
        1.0F
    };
    vk::Rect2D scissor{{0, 0}, swapchain_.image_size()};

    auto vertex_shader = std::make_unique<Shader>("shaders/render_shader.vert.spv", context_);
    auto fragment_shader = std::make_unique<Shader>("shaders/render_shader.frag.spv", context_);
    vertex_shader->AddPushConstantRanges({vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 0,
                                                                sizeof(glm::mat4)}});

    auto [pipeline, pipeline_layout] =
        PipelineBuilder(context_)
            .SetVertexShader(std::move(vertex_shader))
            .SetFragmentShader(std::move(fragment_shader))
            .AddInputBufferDescription(vertex_input_binding_description, vertex_input_attributes)
            .SetInputTopology(vk::PrimitiveTopology::eTriangleList)
            .SetViewport(viewport)
            .SetScissor(scissor)
            .SetFillMode(vk::PolygonMode::eFill)
            .SetColorBlendEnable(false)
            .build(swapchain_.surface_format().format);

    pipelines_.push_back(std::move(pipeline));
    pipeline_layouts_.push_back(std::move(pipeline_layout));
  }
}
}
