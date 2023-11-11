#include "rendering/vulkan/vulkan_renderer.h"
#include "rendering/vulkan/shader.h"
#include "rendering/vulkan/pipeline_builder.h"

#include <absl/status/statusor.h>
#include <absl/log/log.h>

namespace chove::rendering::vulkan {
namespace {

constexpr int kTargetFrameRate = 60;
constexpr long long kTargetFrameTimeNs = 1'000'000'000 / kTargetFrameRate;
constexpr int kPreferredNvidiaBlockSize = 1048576;

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
  auto result = context_.device().waitForFences(*rendering_complete_fence_, true, kTargetFrameTimeNs);
  if (result == vk::Result::eTimeout) {
    LOG(WARNING) << "Previous frame did not present in time.";
    return;
  }
  rendering_command_pool_.reset(vk::CommandPoolResetFlags{});
  context_.device().resetFences(*rendering_complete_fence_);

  result = context_.device().waitForFences(*transfer_complete_fence_, true, kTargetFrameTimeNs);
  if (result == vk::Result::eTimeout) {
    LOG(WARNING) << "Previous frame did not transfer in time.";
    return;
  }
  transfer_command_pool_.reset(vk::CommandPoolResetFlags{});
  context_.device().resetFences(*transfer_complete_fence_);

  transfer_command_buffer().begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  for (const auto &object : objects_) {
    void *buffer_host_memory = object.staging_buffer.GetMappedMemory();
    memcpy(buffer_host_memory,
           object.mesh->vertices.data(),
           object.vertex_buffer.size());
    memcpy(static_cast<char *>(buffer_host_memory) + object.vertex_buffer.size(),
           object.mesh->indices.data(),
           object.index_buffer.size());

    transfer_command_buffer().copyBuffer(object.staging_buffer.buffer(),
                                         object.vertex_buffer.buffer(),
                                         vk::BufferCopy{0, 0, object.vertex_buffer.size()});

    transfer_command_buffer().copyBuffer(object.staging_buffer.buffer(),
                                         object.index_buffer.buffer(),
                                         vk::BufferCopy{object.vertex_buffer.size(), 0, object.index_buffer.size()});

    vk::BufferMemoryBarrier2 vertex_buffer_memory_barrier{
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eMemoryWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eMemoryRead,
        context_.transfer_queue().family_index,
        context_.graphics_queue().family_index,
        object.vertex_buffer.buffer(),
        0,
        object.vertex_buffer.size()
    };
    vk::BufferMemoryBarrier2 index_buffer_memory_barrier{
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eMemoryWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eMemoryRead,
        context_.transfer_queue().family_index,
        context_.graphics_queue().family_index,
        object.index_buffer.buffer(),
        0,
        object.index_buffer.size()
    };
    std::vector<vk::BufferMemoryBarrier2> buffer_memory_barriers{vertex_buffer_memory_barrier,
                                                                 index_buffer_memory_barrier};

    transfer_command_buffer().pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                                  nullptr, buffer_memory_barriers,
                                                                  nullptr});
  }
  transfer_command_buffer().end();

  // submit transfer command buffer
  {
    vk::SemaphoreSubmitInfo signal_semaphore_info{
        *transfer_complete_semaphore_,
        1,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        0,
        nullptr
    };
    vk::CommandBufferSubmitInfo command_buffer_submit_info{*transfer_command_buffer(), 0};
    context_.transfer_queue().queue.submit2(vk::SubmitInfo2{
        vk::SubmitFlags{},
        nullptr,
        command_buffer_submit_info,
        signal_semaphore_info,
    }, *transfer_complete_fence_);
  }

  glm::mat4 view_matrix = scene.camera().GetTransformMatrix();

  // acquire a swapchain image
  auto [image_result, next_image] =
      swapchain_.AcquireNextImage(kTargetFrameTimeNs, *image_acquired_semaphore_, nullptr);
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

  drawing_command_buffer().begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

  // wait for transfers to complete
  {
    for (const auto &object : objects_) {
      vk::BufferMemoryBarrier2 vertex_buffer_memory_barrier{
          vk::PipelineStageFlagBits2::eTransfer,
          vk::AccessFlagBits2::eMemoryWrite,
          vk::PipelineStageFlagBits2::eVertexAttributeInput,
          vk::AccessFlagBits2::eMemoryRead,
          context_.transfer_queue().family_index,
          context_.graphics_queue().family_index,
          object.vertex_buffer.buffer(),
          0,
          object.vertex_buffer.size()
      };
      vk::BufferMemoryBarrier2 index_buffer_memory_barrier{
          vk::PipelineStageFlagBits2::eTransfer,
          vk::AccessFlagBits2::eMemoryWrite,
          vk::PipelineStageFlagBits2::eIndexInput,
          vk::AccessFlagBits2::eMemoryRead,
          context_.transfer_queue().family_index,
          context_.graphics_queue().family_index,
          object.index_buffer.buffer(),
          0,
          object.index_buffer.size()
      };
      std::vector<vk::BufferMemoryBarrier2> buffer_memory_barriers{vertex_buffer_memory_barrier,
                                                                   index_buffer_memory_barrier};

      drawing_command_buffer().pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                                   nullptr, buffer_memory_barriers,
                                                                   nullptr});
    }
  }

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
    drawing_command_buffer().pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                                 nullptr,
                                                                 nullptr,
                                                                 image_memory_barrier,
                                                                 nullptr});
  }

  // begin rendering
  drawing_command_buffer().beginRendering(vk::RenderingInfo{
      vk::RenderingFlags{},
      vk::Rect2D{{0, 0}, swapchain_.image_size()},
      1,
      0,
      color_attachment_info,
      nullptr, nullptr, nullptr
  });

  // bind drawing data
  drawing_command_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelines_[0]);

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
    drawing_command_buffer().clearAttachments(clear_attachment, clear_rect);
  }
  for (const auto &object : objects_) {
    drawing_command_buffer().pushConstants<glm::mat4>(*pipeline_layouts_[0],
                                                      vk::ShaderStageFlagBits::eVertex,
                                                      0,
                                                      view_matrix * object.transform->GetMatrix());
    drawing_command_buffer().bindVertexBuffers(0, object.vertex_buffer.buffer(), {0});
    drawing_command_buffer().bindIndexBuffer(object.index_buffer.buffer(), 0, vk::IndexType::eUint32);
    drawing_command_buffer().drawIndexed(object.mesh->indices.size(), 1, 0, 0, 0);
  }

  // end rendering
  drawing_command_buffer().endRendering();

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
    drawing_command_buffer().pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                                 nullptr,
                                                                 nullptr,
                                                                 image_memory_barrier,
                                                                 nullptr});
  }

  drawing_command_buffer().end();

  // submit command buffer
  {
    std::vector<vk::SemaphoreSubmitInfo> wait_semaphore_infos =
        {{*image_acquired_semaphore_, 1,
          vk::PipelineStageFlagBits2::eTopOfPipe, 0, nullptr},
         {*transfer_complete_semaphore_, 1,
          vk::PipelineStageFlagBits2::eTransfer, 0, nullptr}};
    vk::SemaphoreSubmitInfo signal_semaphore_submit_info
        {*rendering_complete_semaphore_, 1,
         vk::PipelineStageFlagBits2::eBottomOfPipe, 0, nullptr};
    vk::CommandBufferSubmitInfo command_buffer_submit_info{*drawing_command_buffer(), 0, nullptr};
    context_.graphics_queue().queue.submit2(vk::SubmitInfo2{
        vk::SubmitFlags{},
        wait_semaphore_infos,
        command_buffer_submit_info,
        signal_semaphore_submit_info,
        nullptr
    }, *rendering_complete_fence_);
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
    transfer_complete_semaphore_(context_.device(),
                                 vk::SemaphoreCreateInfo{}),
    rendering_complete_fence_(context_.device(),
                              vk::FenceCreateInfo{
                                  vk::FenceCreateFlagBits::eSignaled}),
    transfer_complete_fence_(context_.device(),
                             vk::FenceCreateInfo{
                                 vk::FenceCreateFlagBits::eSignaled}),
    rendering_command_pool_(CreateCommandPool(context_.graphics_queue().family_index, context_.device())),
    transfer_command_pool_(CreateCommandPool(context_.transfer_queue().family_index, context_.device())) {

  auto rendering_command_buffers = AllocateCommandBuffers(context_.device(), rendering_command_pool_);
  auto transfer_command_buffers = AllocateCommandBuffers(context_.device(), transfer_command_pool_);
  command_buffers_.insert(command_buffers_.end(),
                          std::make_move_iterator(rendering_command_buffers.begin()),
                          std::make_move_iterator(rendering_command_buffers.end()));
  command_buffers_.insert(command_buffers_.end(),
                          std::make_move_iterator(transfer_command_buffers.begin()),
                          std::make_move_iterator(transfer_command_buffers.end()));

  VmaAllocatorCreateInfo allocator_create_info = {
      0,
      context_.physical_device(),
      *context_.device(),
      kPreferredNvidiaBlockSize,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      context_.instance(),
      Context::GetVulkanVersion(),
      nullptr
  };
  vmaCreateAllocator(&allocator_create_info, &gpu_memory_allocator_);
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
  objects_.clear();
  pipelines_.clear();
  pipeline_layouts_.clear();

  // Create buffers
  for (const auto &object : scene.objects()) {
    Buffer vertex_buffer{gpu_memory_allocator_,
                         object.mesh->vertices.size() * sizeof(Mesh::Vertex),
                         vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                         context_.graphics_queue().family_index};

    Buffer index_buffer{gpu_memory_allocator_,
                        object.mesh->indices.size() * sizeof(uint32_t),
                        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        context_.graphics_queue().family_index};

    Buffer staging_buffer{gpu_memory_allocator_,
                          vertex_buffer.size() + index_buffer.size(),
                          vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          context_.transfer_queue().family_index};

    objects_.push_back({object.mesh, std::move(vertex_buffer), std::move(staging_buffer), std::move(index_buffer),
                        object.transform});
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

VulkanRenderer::~VulkanRenderer() {
  if ((VkDevice) *context_.device() != VK_NULL_HANDLE) {
    context_.device().waitIdle();
  }
  if (!objects_.empty()) {
    objects_.clear();
  }
  if (gpu_memory_allocator_ != nullptr) {
    vmaDestroyAllocator(gpu_memory_allocator_);
  }
}

VulkanRenderer::VulkanRenderer(VulkanRenderer &&other) noexcept\
: context_(std::move(other.context_)),
  swapchain_(std::move(other.swapchain_)),
  image_acquired_semaphore_(std::move(other.image_acquired_semaphore_)),
  rendering_complete_semaphore_(std::move(other.rendering_complete_semaphore_)),
  transfer_complete_semaphore_(std::move(other.transfer_complete_semaphore_)),
  rendering_complete_fence_(std::move(other.rendering_complete_fence_)),
  transfer_complete_fence_(std::move(other.transfer_complete_fence_)),
  rendering_command_pool_(std::move(other.rendering_command_pool_)),
  transfer_command_pool_(std::move(other.transfer_command_pool_)),
  command_buffers_(std::move(other.command_buffers_)),
  objects_(std::move(other.objects_)),
  pipelines_(std::move(other.pipelines_)),
  pipeline_layouts_(std::move(other.pipeline_layouts_)) {
  gpu_memory_allocator_ = other.gpu_memory_allocator_;
  other.gpu_memory_allocator_ = nullptr;
}
VulkanRenderer &VulkanRenderer::operator=(VulkanRenderer &&other) noexcept {
  context_ = std::move(other.context_);
  swapchain_ = std::move(other.swapchain_);
  image_acquired_semaphore_ = std::move(other.image_acquired_semaphore_);
  rendering_complete_semaphore_ = std::move(other.rendering_complete_semaphore_);
  transfer_complete_semaphore_ = std::move(other.transfer_complete_semaphore_);
  rendering_complete_fence_ = std::move(other.rendering_complete_fence_);
  transfer_complete_fence_ = std::move(other.transfer_complete_fence_);
  rendering_command_pool_ = std::move(other.rendering_command_pool_);
  transfer_command_pool_ = std::move(other.transfer_command_pool_);
  command_buffers_ = std::move(other.command_buffers_);
  objects_ = std::move(other.objects_);
  pipelines_ = std::move(other.pipelines_);
  pipeline_layouts_ = std::move(other.pipeline_layouts_);
  gpu_memory_allocator_ = other.gpu_memory_allocator_;
  other.gpu_memory_allocator_ = nullptr;
  return *this;
}

}
