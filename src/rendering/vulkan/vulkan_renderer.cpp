#include "rendering/vulkan/vulkan_renderer.h"
#include "rendering/vulkan/shader.h"
#include "rendering/vulkan/pipeline_builder.h"

#include <absl/status/statusor.h>
#include <absl/log/log.h>

namespace chove::rendering::vulkan {
namespace {
constexpr int kTextureCount = 7;
constexpr int kTargetFrameRate = 60;
constexpr long long kTargetFrameTimeNs = 1'000'000'000 / kTargetFrameRate;
struct MaterialData {
  float shininess;
  float optical_density;
  float dissolve;
  float filler = 0.0F;
  glm::vec4 transmission_filter_color;
  glm::vec4 ambient_color;
  glm::vec4 diffuse_color;
  glm::vec4 specular_color;
};

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

    MaterialData material_data{
        object.mesh->material.shininess,
        object.mesh->material.optical_density,
        object.mesh->material.dissolve,
        0.0F,
        glm::vec4(object.mesh->material.transmission_filter_color, 1.0F),
        glm::vec4(object.mesh->material.ambient_color, 1.0F),
        glm::vec4(object.mesh->material.diffuse_color, 1.0F),
        glm::vec4(object.mesh->material.specular_color, 1.0F)
    };
    memcpy(static_cast<char *>(buffer_host_memory) + object.vertex_buffer.size() + object.index_buffer.size(),
           &material_data,
           sizeof(material_data));

    transfer_command_buffer().copyBuffer(object.staging_buffer.buffer(),
                                         object.vertex_buffer.buffer(),
                                         vk::BufferCopy{0, 0, object.vertex_buffer.size()});

    transfer_command_buffer().copyBuffer(object.staging_buffer.buffer(),
                                         object.index_buffer.buffer(),
                                         vk::BufferCopy{object.vertex_buffer.size(), 0, object.index_buffer.size()});

    transfer_command_buffer().copyBuffer(object.staging_buffer.buffer(),
                                         object.uniform_buffer.buffer(),
                                         vk::BufferCopy{object.vertex_buffer.size() + object.index_buffer.size(),
                                                        0,
                                                        object.uniform_buffer.size()});

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
                                            },
                                            *transfer_complete_fence_);
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
      vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 0.0F}}
  };

  vk::RenderingAttachmentInfo depth_attachment_info{
      depth_buffer_.view(),
      vk::ImageLayout::eDepthAttachmentOptimal,
      vk::ResolveModeFlagBits::eNone,
      depth_buffer_.view(),
      vk::ImageLayout::eDepthAttachmentOptimal,
      vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eStore,
      vk::ClearDepthStencilValue{1.0F, 0}
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

  // transition images to proper formats
  {
    vk::ImageMemoryBarrier2 color_attachment_memory_barrier
        {vk::PipelineStageFlagBits2::eTopOfPipe, {},
         vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
         vk::ImageLayout::eUndefined,
         vk::ImageLayout::eColorAttachmentOptimal,
         context_.graphics_queue().family_index,
         context_.graphics_queue().family_index,
         next_image.image,
         vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
         nullptr};

    vk::ImageMemoryBarrier2 depth_buffer_memory_barrier
        {vk::PipelineStageFlagBits2::eTopOfPipe, {},
         vk::PipelineStageFlagBits2::eEarlyFragmentTests,
         vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
         vk::ImageLayout::eUndefined,
         vk::ImageLayout::eDepthAttachmentOptimal,
         context_.graphics_queue().family_index,
         context_.graphics_queue().family_index,
         depth_buffer_.image(),
         vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1},
         nullptr};

    std::vector<vk::ImageMemoryBarrier2> image_memory_barriers{color_attachment_memory_barrier,
                                                               depth_buffer_memory_barrier};

    drawing_command_buffer().pipelineBarrier2(vk::DependencyInfo{vk::DependencyFlags{},
                                                                 nullptr,
                                                                 nullptr,
                                                                 image_memory_barriers,
                                                                 nullptr});
  }

  // begin rendering
  drawing_command_buffer().beginRendering(vk::RenderingInfo{
      vk::RenderingFlags{},
      vk::Rect2D{{0, 0}, swapchain_.image_size()},
      1,
      0,
      color_attachment_info,
      &depth_attachment_info,
      nullptr, nullptr
  });

  // register draw commands and bind drawing data
  {
    vk::ClearAttachment clear_color_attachment{
        vk::ImageAspectFlagBits::eColor,
        0,
        vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 0.0F}}
    };
    vk::ClearRect clear_rect{
        vk::Rect2D{{0, 0}, swapchain_.image_size()},
        0,
        1
    };
    drawing_command_buffer().clearAttachments(clear_color_attachment, clear_rect);
  }

  for (const auto &object : objects_) {
    drawing_command_buffer().pushConstants<glm::mat4>(*pipeline_layouts_[0],
                                                      vk::ShaderStageFlagBits::eVertex,
                                                      0,
                                                      view_matrix * object.transform->GetMatrix());
    drawing_command_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelines_[object.pipeline_index]);
    drawing_command_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                *pipeline_layouts_[object.pipeline_index],
                                                0,
                                                *descriptor_sets_[object.pipeline_index],
                                                nullptr);
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
                                            },
                                            *rendering_complete_fence_);
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
  context_.device().waitIdle();
}

VulkanRenderer::VulkanRenderer(Context context,
                               Swapchain swapchain,
                               Image depth_buffer,
                               vk::raii::Semaphore image_acquired_semaphore,
                               vk::raii::Semaphore rendering_complete_semaphore,
                               vk::raii::Semaphore transfer_complete_semaphore,
                               vk::raii::Fence rendering_complete_fence,
                               vk::raii::Fence transfer_complete_fence,
                               vk::raii::CommandPool rendering_command_pool,
                               vk::raii::CommandPool transfer_command_pool,
                               std::vector<vk::raii::CommandBuffer> command_buffers,
                               Allocator gpu_memory_allocator) :
    context_(std::move(context)),
    swapchain_(std::move(swapchain)),
    depth_buffer_(std::move(depth_buffer)),
    image_acquired_semaphore_(std::move(image_acquired_semaphore)),
    rendering_complete_semaphore_(std::move(rendering_complete_semaphore)),
    transfer_complete_semaphore_(std::move(transfer_complete_semaphore)),
    rendering_complete_fence_(std::move(rendering_complete_fence)),
    transfer_complete_fence_(std::move(transfer_complete_fence)),
    rendering_command_pool_(std::move(rendering_command_pool)),
    transfer_command_pool_(std::move(transfer_command_pool)),
    command_buffers_(std::move(command_buffers)),
    gpu_memory_allocator_(std::move(gpu_memory_allocator)) {}

absl::StatusOr<VulkanRenderer> VulkanRenderer::Create(Window &window) {
  absl::StatusOr<Context> context = Context::CreateContext(window);
  if (!context.ok()) {
    return context.status();
  }

  absl::StatusOr<Swapchain> swapchain = Swapchain::CreateSwapchain(*context, window, 3);
  if (!swapchain.ok()) {
    return swapchain.status();
  }

  vk::raii::Semaphore image_acquired_semaphore(context->device(), vk::SemaphoreCreateInfo{});
  vk::raii::Semaphore rendering_complete_semaphore(context->device(), vk::SemaphoreCreateInfo{});
  vk::raii::Semaphore transfer_complete_semaphore(context->device(), vk::SemaphoreCreateInfo{});
  vk::raii::Fence rendering_complete_fence(context->device(),
                                           vk::FenceCreateInfo{
                                               vk::FenceCreateFlagBits::eSignaled});
  vk::raii::Fence transfer_complete_fence(context->device(),
                                          vk::FenceCreateInfo{
                                              vk::FenceCreateFlagBits::eSignaled});
  vk::raii::CommandPool
      rendering_command_pool(CreateCommandPool(context->graphics_queue().family_index, context->device()));
  vk::raii::CommandPool
      transfer_command_pool(CreateCommandPool(context->transfer_queue().family_index, context->device()));

  std::vector<vk::raii::CommandBuffer>
      rendering_command_buffers = AllocateCommandBuffers(context->device(), rendering_command_pool);
  std::vector<vk::raii::CommandBuffer>
      transfer_command_buffers = AllocateCommandBuffers(context->device(), transfer_command_pool);

  std::vector<vk::raii::CommandBuffer> command_buffers;
  command_buffers.insert(command_buffers.end(),
                         std::make_move_iterator(rendering_command_buffers.begin()),
                         std::make_move_iterator(rendering_command_buffers.end()));
  command_buffers.insert(command_buffers.end(),
                         std::make_move_iterator(transfer_command_buffers.begin()),
                         std::make_move_iterator(transfer_command_buffers.end()));

  Allocator gpu_memory_allocator{*context};

  Image depth_buffer = Image::CreateImage(*context,
                                          gpu_memory_allocator.allocator(),
                                          vk::Format::eD32Sfloat,
                                          vk::ImageAspectFlagBits::eDepth,
                                          vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                          swapchain->image_size(),
                                          context->graphics_queue().family_index);

  return VulkanRenderer{*std::move(context), *std::move(swapchain),
                        std::move(depth_buffer),
                        std::move(image_acquired_semaphore),
                        std::move(rendering_complete_semaphore),
                        std::move(transfer_complete_semaphore),
                        std::move(rendering_complete_fence),
                        std::move(transfer_complete_fence),
                        std::move(rendering_command_pool),
                        std::move(transfer_command_pool),
                        std::move(command_buffers),
                        std::move(gpu_memory_allocator)};
}

void VulkanRenderer::SetupScene(const objects::Scene &scene) {
  objects_.clear();
  pipelines_.clear();
  pipeline_layouts_.clear();
  descriptor_set_layouts_.clear();
  descriptor_pools_.clear();
  descriptor_sets_.clear();

  // Create buffers
  for (const auto &object : scene.objects()) {
    Buffer vertex_buffer{gpu_memory_allocator_.allocator(),
                         object.mesh->vertices.size() * sizeof(Mesh::Vertex),
                         vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                         context_.graphics_queue().family_index};

    Buffer index_buffer{gpu_memory_allocator_.allocator(),
                        object.mesh->indices.size() * sizeof(uint32_t),
                        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        context_.graphics_queue().family_index};

    Buffer uniform_buffer{gpu_memory_allocator_.allocator(),
                          sizeof(MaterialData),
                          vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          context_.graphics_queue().family_index};

    Buffer staging_buffer{gpu_memory_allocator_.allocator(),
                          vertex_buffer.size() + index_buffer.size() + uniform_buffer.size(),
                          vk::BufferUsageFlagBits::eTransferSrc,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          context_.transfer_queue().family_index};

    std::unique_ptr<Texture> diffuse_texture = nullptr;
//    if (object.mesh->material.diffuse_texture.has_value()) {
//      diffuse_texture = std::move(Texture::CreateTexture(context_,
//                                                         gpu_memory_allocator_.allocator(),
//                                                         context_.graphics_queue().family_index,
//                                                         object.mesh->material.diffuse_texture.value()));
//    }

    objects_.push_back({object.mesh, std::move(vertex_buffer), std::move(staging_buffer), std::move(index_buffer),
                        std::move(uniform_buffer), std::move(diffuse_texture), object.transform, 0});
  }

  // Create pipelines
  for (auto &object : objects_) {
    vk::VertexInputBindingDescription vertex_input_binding_description{
        0,
        sizeof(Mesh::Vertex),
        vk::VertexInputRate::eVertex
    };
    vk::VertexInputAttributeDescription vertex_input_position_description{
        0,
        0,
        vk::Format::eR32G32B32Sfloat,
        0
    };
    vk::VertexInputAttributeDescription vertex_input_normal_description{
        1,
        0,
        vk::Format::eR32G32B32Sfloat,
        static_cast<uint32_t>(offsetof(Mesh::Vertex, normal))
    };
    vk::VertexInputAttributeDescription vertex_input_texcoord_description{
        2,
        0,
        vk::Format::eR32G32Sfloat,
        static_cast<uint32_t>(offsetof(Mesh::Vertex, texcoord))
    };

    const std::vector<vk::VertexInputAttributeDescription>
        vertex_input_attributes{vertex_input_position_description,
                                vertex_input_normal_description,
                                vertex_input_texcoord_description};

    const vk::Viewport viewport{
        0.0F,
        0.0F,
        static_cast<float>(swapchain_.image_size().width),
        static_cast<float>(swapchain_.image_size().height),
        0.0F,
        1.0F
    };
    vk::Rect2D scissor{{0, 0}, swapchain_.image_size()};

    auto vertex_shader = std::make_unique<Shader>("shaders/render_shader.vert.spv", context_);
    auto fragment_shader = std::make_unique<Shader>("shaders/render_shader.frag.spv", context_);
    vertex_shader->AddPushConstantRanges({vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 0,
                                                                sizeof(glm::mat4)}});

    std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_bindings;
    descriptor_set_layout_bindings.emplace_back(
        0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex,
        nullptr);
//    descriptor_set_layout_bindings.emplace_back(
//        1,
//        vk::DescriptorType::eCombinedImageSampler,
//        kTextureCount,
//        vk::ShaderStageFlagBits::eVertex,
//        nullptr);

    descriptor_set_layouts_.push_back(vertex_shader->AddDescriptorSetLayout(descriptor_set_layout_bindings));

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
            .build(swapchain_.surface_format().format, depth_buffer_.format());

    pipelines_.push_back(std::move(pipeline));
    pipeline_layouts_.push_back(std::move(pipeline_layout));
    object.pipeline_index = static_cast<int>(pipelines_.size()) - 1;
  }

  std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
  descriptor_pool_sizes.emplace_back(vk::DescriptorType::eUniformBuffer, 1);
  descriptor_pool_sizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, kTextureCount);
  vk::raii::DescriptorPool pool{context_.device(), vk::DescriptorPoolCreateInfo{
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      static_cast<uint32_t>(objects_.size() + 100),
      descriptor_pool_sizes
  }};
  descriptor_pools_.push_back(std::move(pool));

  descriptor_sets_ = context_.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
      *descriptor_pools_[0],
      descriptor_set_layouts_,
      nullptr
  });

  {
    transfer_command_buffer().begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    for (const auto &object : objects_) {
      void *buffer_host_memory = object.staging_buffer.GetMappedMemory();
      MaterialData material_data{
          object.mesh->material.shininess,
          object.mesh->material.optical_density,
          object.mesh->material.dissolve,
          0.0F,
          glm::vec4(object.mesh->material.transmission_filter_color, 1.0F),
          glm::vec4(object.mesh->material.ambient_color, 1.0F),
          glm::vec4(object.mesh->material.diffuse_color, 1.0F),
          glm::vec4(object.mesh->material.specular_color, 1.0F)
      };
      memcpy(static_cast<char *>(buffer_host_memory) + object.vertex_buffer.size() + object.index_buffer.size(),
             &material_data,
             sizeof(material_data));

      transfer_command_buffer().copyBuffer(object.staging_buffer.buffer(),
                                           object.uniform_buffer.buffer(),
                                           vk::BufferCopy{object.vertex_buffer.size() + object.index_buffer.size(),
                                                          0,
                                                          object.uniform_buffer.size()});
      vk::BufferMemoryBarrier2 uniform_buffer_memory_barrier{
          vk::PipelineStageFlagBits2::eTransfer,
          vk::AccessFlagBits2::eMemoryWrite,
          vk::PipelineStageFlagBits2::eBottomOfPipe,
          vk::AccessFlagBits2::eMemoryRead,
          context_.transfer_queue().family_index,
          context_.graphics_queue().family_index,
          object.uniform_buffer.buffer(),
          0,
          object.uniform_buffer.size()
      };
      std::vector<vk::BufferMemoryBarrier2> buffer_memory_barriers{uniform_buffer_memory_barrier};

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
                                              },
                                              *transfer_complete_fence_);
    }
  }

  for (const auto &object : objects_) {
    vk::DescriptorBufferInfo descriptor_buffer_info{
        object.uniform_buffer.buffer(),
        0,
        object.uniform_buffer.size()
    };
    context_.device().updateDescriptorSets(vk::WriteDescriptorSet{
        *descriptor_sets_[object.pipeline_index],
        0,
        0,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        descriptor_buffer_info,
        nullptr
    }, nullptr);
  }

  for (const auto &object : objects_) {
    //
  }
}

VulkanRenderer::~VulkanRenderer() {
  if ((VkDevice) *context_.device() != VK_NULL_HANDLE) {
    context_.device().waitIdle();
  }
}

VulkanRenderer::VulkanRenderer(VulkanRenderer &&other) noexcept:
    context_(std::move(other.context_)),
    swapchain_(std::move(other.swapchain_)),
    depth_buffer_(std::move(other.depth_buffer_)),
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
    pipeline_layouts_(std::move(other.pipeline_layouts_)),
    gpu_memory_allocator_(std::move(other.gpu_memory_allocator_)) {}

VulkanRenderer &VulkanRenderer::operator=(VulkanRenderer &&other) noexcept {
  context_ = std::move(other.context_);
  swapchain_ = std::move(other.swapchain_);
  depth_buffer_ = std::move(other.depth_buffer_);
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
  gpu_memory_allocator_ = std::move(other.gpu_memory_allocator_);
  return *this;
}
}
