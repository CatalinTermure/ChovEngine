// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <filesystem>
#include <vector>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <absl/log/log.h>
#include <absl/log/check.h>
#include <absl/log/initialize.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>

#include "rendering/camera.h"
#include "rendering/context.h"
#include "rendering/pipeline_builder.h"
#include "rendering/swapchain.h"

namespace chove {

// This class should store an object
// What is an object made up of?
// 1) It has vertices
// 2) It has indices
// 3) It also has a "material". Which has:
// 3.1) A texture associated with it
// 3.2) Also a color, which may be combined with the texture color
// 3.3) Also a shader/pipeline!!!

struct Vertex {
  glm::vec4 position;
  glm::vec2 uv;
  [[nodiscard]] static constexpr std::array<vk::VertexInputAttributeDescription, 2> attributes() {
    return {
        vk::VertexInputAttributeDescription{
            0,
            0,
            vk::Format::eR32G32B32A32Sfloat,
            0
        },
        vk::VertexInputAttributeDescription{
            0,
            0,
            vk::Format::eR32G32B32A32Sfloat,
            sizeof(glm::vec4)
        }
    };
  }
};

struct Texture {};

struct Material {
  Texture *texture;
  glm::vec4 color;
  std::shared_ptr<vk::raii::Pipeline> pipeline;
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<int> indices;

  Material *material;
};

struct Transform {
  glm::vec3 location;
  glm::quat rotation;
  glm::vec3 scale;

  // While not necessary for the transform matrix, we also store the velocity and angular velocity of the object so that
  // the structure is more cache friendly.

  glm::vec3 velocity;
  glm::quat angular_velocity;

  [[nodiscard]] glm::mat4 GetMatrix() const {
    return glm::translate(glm::scale(glm::toMat4(rotation), scale), location);
  }
};

class GameObject {
 public:
  [[nodiscard]] const Mesh &mesh() const { return *mesh_; }
  [[nodiscard]] Transform &transform() { return *transform_; }

 private:
  Mesh *mesh_;
  std::unique_ptr<Transform> transform_;
};

// TODO: A GameObjectBatch is a batch of game objects that are similar from a GPU point of view.
// This means that they may be:
// 1) The same Mesh, but multiple instances of it.
// 2) Different Mesh, but the same Material.
// 3) Different Material, but the same pipeline.
class GameObjectBatch {
 public:
  void render();
 private:
  std::vector<GameObject> objects_;
};

class Scene {
 public:
  void render() {
    for (auto &batch : object_batches_) {
      batch.render();
    }
  }
 private:
  void acquire_image();
  std::vector<GameObjectBatch> object_batches_;
};

class Application {
  //
};

}  // namespace chove

class StdoutLogSink : public absl::LogSink {
  void Send(const absl::LogEntry &entry) override {
    std::cout << entry.text_message_with_prefix_and_newline();
  }
};

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

struct Vertex {
  glm::vec4 position;
  glm::vec4 color;
};

const std::vector<Vertex> triangle = {
    {{-1.0F, 0.0F, 1.0F, 1.0F}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{1.0F, 0.0F, 1.0F, 1.0F}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{0.0F, -1.0F, 0.0F, 1.0F}, {0.0f, 0.0f, 1.0f, 1.0f}}
};

constexpr int target_frame_rate = 60;
constexpr long long target_frame_time_ns = 1'000'000'000 / target_frame_rate;
constexpr std::chrono::duration target_frame_time = std::chrono::nanoseconds(target_frame_time_ns);

int main() {
  StdoutLogSink log_sink{};
  absl::AddLogSink(&log_sink);
  absl::InitializeLog();

  chove::rendering::Context context = []() {
    absl::StatusOr<chove::rendering::Context> context = chove::rendering::Context::CreateContext();
    CHECK_OK(context) << "Could not create render context.";
    return std::move(*context);
  }();

  const chove::rendering::Swapchain swapchain = [&context]() {
    absl::StatusOr<chove::rendering::Swapchain> swapchain = chove::rendering::Swapchain::CreateSwapchain(context, 2);
    CHECK_OK(swapchain) << "Could not create swapchain.";
    return std::move(*swapchain);
  }();

  const vk::raii::CommandPool command_pool = CreateCommandPool(context.graphics_queue().family_index, context.device());
  const std::vector<vk::raii::CommandBuffer> command_buffers = AllocateCommandBuffers(context.device(), command_pool);

  chove::rendering::Camera camera(
      {0.0F, 0.0F, -1.0F, 1.0F},
      {0.0F, 0.0F, 1.0F},
      glm::radians(45.0F),
      static_cast<float>(swapchain.image_size().width) / static_cast<float>(swapchain.image_size().height),
      0.1F,
      10.0F);

  vk::raii::Semaphore image_acquired_semaphore{context.device(), vk::SemaphoreCreateInfo{}};
  vk::raii::Semaphore rendering_complete_semaphore{context.device(), vk::SemaphoreCreateInfo{}};
  vk::raii::Fence image_presented_fence{context.device(), vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}};

  vk::VertexInputBindingDescription vertex_input_binding_description{
      0,
      sizeof(Vertex),
      vk::VertexInputRate::eVertex
  };
  vk::VertexInputAttributeDescription vertex_input_position_description{
      0,
      0,
      vk::Format::eR32G32B32A32Sfloat,
      static_cast<uint32_t>(offsetof(Vertex, position))
  };
  vk::VertexInputAttributeDescription vertex_input_color_description{
      1,
      0,
      vk::Format::eR32G32B32A32Sfloat,
      static_cast<uint32_t>(offsetof(Vertex, color))
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
      static_cast<float>(swapchain.image_size().width),
      static_cast<float>(swapchain.image_size().height),
      0.1F,
      1.0F
  };
  vk::Rect2D scissor{{0, 0}, swapchain.image_size()};

  auto vertex_shader = std::make_unique<chove::rendering::Shader>("shaders/render_shader.vert.spv", context);
  auto fragment_shader = std::make_unique<chove::rendering::Shader>("shaders/render_shader.frag.spv", context);
  vertex_shader->AddPushConstantRanges({vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 16,
                                                              sizeof(glm::mat4)}});
  fragment_shader->AddPushConstantRanges({vk::PushConstantRange{vk::ShaderStageFlagBits::eFragment, 0,
                                                                sizeof(glm::vec2)}});

  auto [graphics_pipeline, pipeline_layout] =
      chove::rendering::PipelineBuilder(context)
          .SetVertexShader(std::move(vertex_shader))
          .SetFragmentShader(std::move(fragment_shader))
          .AddInputBufferDescription(vertex_input_binding_description, vertex_input_attributes)
          .SetInputTopology(vk::PrimitiveTopology::eTriangleList)
          .SetViewport(viewport)
          .SetScissor(scissor)
          .SetFillMode(vk::PolygonMode::eFill)
          .SetColorBlendEnable(false)
          .build(swapchain.surface_format().format);

  std::vector<glm::vec2> window_size = {glm::vec2{swapchain.image_size().width, swapchain.image_size().height}};

  vk::raii::Buffer triangle_buffer{context.device(), vk::BufferCreateInfo{
      vk::BufferCreateFlags{},
      sizeof(Vertex) * triangle.size(),
      vk::BufferUsageFlagBits::eVertexBuffer,
      vk::SharingMode::eExclusive,
      context.graphics_queue().family_index,
      nullptr
  }};

  vk::MemoryRequirements triangle_buffer_memory_requirements = triangle_buffer.getMemoryRequirements();
  LOG(INFO) << std::format("These are the memory requirements for triangle buffer: {}.",
                           to_string(vk::MemoryPropertyFlags{triangle_buffer_memory_requirements.memoryTypeBits}));
  vk::MemoryPropertyFlags desired_memory_properties{triangle_buffer_memory_requirements.memoryTypeBits & 0x7};

  uint32_t memory_type_index;
  vk::PhysicalDeviceMemoryProperties available_memory_properties = context.physical_device().getMemoryProperties();
  for (uint32_t i = 0; i < available_memory_properties.memoryTypeCount; i++) {
    if ((available_memory_properties.memoryTypes[i].propertyFlags & desired_memory_properties)
        == desired_memory_properties) {
      memory_type_index = i;
      break;
    }
  }

  vk::raii::DeviceMemory triangle_buffer_memory = context.device().allocateMemory(vk::MemoryAllocateInfo{
      triangle_buffer_memory_requirements.size,
      memory_type_index
  });
  triangle_buffer.bindMemory(*triangle_buffer_memory, 0);

  void *triangle_buffer_host_memory =
      triangle_buffer_memory.mapMemory(0, triangle_buffer_memory_requirements.size, vk::MemoryMapFlags{});

  memcpy(triangle_buffer_host_memory, triangle.data(), sizeof(Vertex) * triangle.size());

  triangle_buffer_memory.unmapMemory();

  bool should_app_close = false;
  while (!should_app_close) {
    auto start_frame_time = std::chrono::high_resolution_clock::now();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        should_app_close = true;
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:should_app_close = true;
            break;
          case SDLK_w:camera.Move(chove::rendering::Camera::Direction::eForward, 0.1F);
            break;
          case SDLK_a:camera.Move(chove::rendering::Camera::Direction::eLeft, 0.1F);
            break;
          case SDLK_s:camera.Move(chove::rendering::Camera::Direction::eBackward, 0.1F);
            break;
          case SDLK_d:camera.Move(chove::rendering::Camera::Direction::eRight, 0.1F);
            break;
          case SDLK_LEFT:camera.Rotate(chove::rendering::Camera::RotationDirection::eLeft, 10.0F);
            break;
          case SDLK_RIGHT:camera.Rotate(chove::rendering::Camera::RotationDirection::eRight, 10.0F);
            break;
          case SDLK_UP:camera.Rotate(chove::rendering::Camera::RotationDirection::eUpward, 10.0F);
            break;
          case SDLK_DOWN:camera.Rotate(chove::rendering::Camera::RotationDirection::eDownward, 10.0F);
            break;
          default:LOG(INFO) << std::format("Camera position is: ({},{},{})",
                                           camera.position().x,
                                           camera.position().y,
                                           camera.position().z);
            LOG(INFO) << std::format("Camera look direction is: ({},{},{})",
                                     camera.look_direction().x,
                                     camera.look_direction().y,
                                     camera.look_direction().z);

            for (auto point : triangle) {
              glm::vec4 new_point = camera.GetTransformMatrix() * point.position;
              new_point /= new_point.w;
              LOG(INFO) << std::format("Point is: ({},{},{},{})",
                                       new_point.x,
                                       new_point.y,
                                       new_point.z,
                                       new_point.w);
            }
            break;
        }
      }
    }

    glm::mat4 view_matrix = camera.GetTransformMatrix();

    // acquire a swapchain image
    auto [image_result, next_image] =
        swapchain.AcquireNextImage(target_frame_time_ns, *image_acquired_semaphore, nullptr);
    if (image_result == vk::Result::eTimeout) {
      continue;
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

    auto result = context.device().waitForFences(*image_presented_fence, true, target_frame_time_ns);
    if (result == vk::Result::eTimeout) {
      LOG(WARNING) << "Previous frame did not present in time.";
      continue;
    }
    context.device().resetFences(*image_presented_fence);
    command_buffers[0].begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    // transition image to a format that can be rendered to
    {
      vk::ImageMemoryBarrier2 image_memory_barrier
          {vk::PipelineStageFlagBits2::eTopOfPipe, {},
           vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
           vk::ImageLayout::eUndefined,
           vk::ImageLayout::eColorAttachmentOptimal,
           context.graphics_queue().family_index,
           context.graphics_queue().family_index,
           next_image.image,
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
        vk::Rect2D{{0, 0}, swapchain.image_size()},
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
                                                window_size);
    command_buffers[0].pushConstants<glm::mat4>(*pipeline_layout,
                                                vk::ShaderStageFlagBits::eVertex,
                                                16,
                                                view_matrix);

    command_buffers[0].bindVertexBuffers(0, {*triangle_buffer}, {0});

    // register draw commands
    {
      vk::ClearAttachment clear_attachment{
          vk::ImageAspectFlagBits::eColor,
          0,
          vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 0.0F}}
      };
      vk::ClearRect clear_rect{
          vk::Rect2D{{0, 0}, swapchain.image_size()},
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
           context.graphics_queue().family_index,
           context.graphics_queue().family_index,
           next_image.image,
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
      context.graphics_queue().queue.submit2(vk::SubmitInfo2{
          vk::SubmitFlags{},
          wait_semaphore_submit_info,
          command_buffer_submit_info,
          signal_semaphore_submit_info,
          nullptr
      }, *image_presented_fence);
    }

    // present the swapchain image
    {
      const vk::SwapchainKHR swapchain_khr = swapchain.swapchain_khr();
      result = context.graphics_queue().queue.presentKHR(vk::PresentInfoKHR{*rendering_complete_semaphore,
                                                                            swapchain_khr,
                                                                            next_image.index,
                                                                            nullptr});
      if (result != vk::Result::eSuccess) {
        LOG(WARNING) << "Presenting swapchain image failed.";
        break;
      }
    }

    auto end_frame_time = std::chrono::high_resolution_clock::now();
    if (end_frame_time - start_frame_time > target_frame_time) {
      LOG(INFO) <<
                std::format("Frame time: {} ns.",
                            duration_cast<std::chrono::nanoseconds>(end_frame_time - start_frame_time).count());
    }
  }

  context.device().waitIdle();
  SDL_Quit();
  return 0;
}
