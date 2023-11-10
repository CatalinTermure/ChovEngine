// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <filesystem>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <absl/log/log.h>
#include <absl/log/initialize.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>

#include "rendering/camera.h"
#include "rendering/renderer.h"
#include "rendering/vulkan/vulkan_renderer.h"
#include "rendering/window.h"
#include "rendering/mesh.h"
#include "objects/scene.h"

using chove::objects::Scene;
using chove::objects::Transform;
using chove::rendering::Camera;
using chove::rendering::Window;
using chove::rendering::Mesh;
using chove::rendering::Renderer;
using chove::rendering::vulkan::VulkanRenderer;

class StdoutLogSink : public absl::LogSink {
  void Send(const absl::LogEntry &entry) override {
    std::cout << entry.text_message_with_prefix_and_newline();
  }
};

struct Vertex {
  glm::vec4 position;
  glm::vec4 color;
};

constexpr int target_frame_rate = 60;
constexpr long long target_frame_time_ns = 1'000'000'000 / target_frame_rate;
constexpr std::chrono::duration target_frame_time = std::chrono::nanoseconds(target_frame_time_ns);

int main() {
  StdoutLogSink log_sink{};
  absl::AddLogSink(&log_sink);
  absl::InitializeLog();

  Window window;

  VulkanRenderer renderer = [&window]() {
    absl::StatusOr<VulkanRenderer> renderer = VulkanRenderer::Create(window);
    if (!renderer.ok()) {
      LOG(FATAL) << renderer.status();
    }
    return *std::move(renderer);
  }();

  Mesh mesh1;
  mesh1.vertices.push_back({{-1.0F, 0.0F, 1.0F, 1.0F}, {1.0f, 0.0f, 0.0f, 1.0f}});
  mesh1.vertices.push_back({{1.0F, 0.0F, 1.0F, 1.0F}, {0.0f, 1.0f, 0.0f, 1.0f}});
  mesh1.vertices.push_back({{0.0F, -1.0F, 0.0F, 1.0F}, {0.0f, 0.0f, 1.0f, 1.0f}});

  Scene scene;
  scene.camera() = Camera(
      {0.0F, 0.0F, -1.0F, 1.0F},
      {0.0F, 0.0F, 1.0F},
      glm::radians(45.0F),
      static_cast<float>(window.width()) / static_cast<float>(window.height()),
      0.1F,
      10.0F);

  scene.AddObject(mesh1, Transform{
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::identity<glm::quat>(),
      glm::vec3(1.0f, 1.0f, 1.0f)
  });

  Mesh mesh2 = mesh1;
  mesh2.vertices[2].position = {0.0F, 1.0F, 0.0F, 1.0F};
  scene.AddObject(mesh2, Transform{
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::identity<glm::quat>(),
      glm::vec3(1.0f, 1.0f, 1.0f)
  });

  renderer.SetupScene(scene);

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
          case SDLK_w:scene.camera().Move(chove::rendering::Camera::Direction::eForward, 0.1F);
            break;
          case SDLK_a:scene.camera().Move(chove::rendering::Camera::Direction::eLeft, 0.1F);
            break;
          case SDLK_s:scene.camera().Move(chove::rendering::Camera::Direction::eBackward, 0.1F);
            break;
          case SDLK_d:scene.camera().Move(chove::rendering::Camera::Direction::eRight, 0.1F);
            break;
          case SDLK_LEFT:scene.camera().Rotate(chove::rendering::Camera::RotationDirection::eLeft, 10.0F);
            break;
          case SDLK_RIGHT:scene.camera().Rotate(chove::rendering::Camera::RotationDirection::eRight, 10.0F);
            break;
          case SDLK_UP:scene.camera().Rotate(chove::rendering::Camera::RotationDirection::eUpward, 10.0F);
            break;
          case SDLK_DOWN:scene.camera().Rotate(chove::rendering::Camera::RotationDirection::eDownward, 10.0F);
            break;
          default:LOG(INFO) << std::format("Camera position is: ({},{},{})",
                                           scene.camera().position().x,
                                           scene.camera().position().y,
                                           scene.camera().position().z);
            LOG(INFO) << std::format("Camera look direction is: ({},{},{})",
                                     scene.camera().look_direction().x,
                                     scene.camera().look_direction().y,
                                     scene.camera().look_direction().z);

            for (auto point : mesh1.vertices) {
              glm::vec4 new_point = scene.camera().GetTransformMatrix() * point.position;
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

    renderer.Render(scene);

    auto end_frame_time = std::chrono::high_resolution_clock::now();
    if (end_frame_time - start_frame_time > target_frame_time) {
      LOG(INFO) <<
                std::format("Frame time: {} ns.",
                            duration_cast<std::chrono::nanoseconds>(end_frame_time - start_frame_time).count());
    }
  }

  SDL_Quit();
  return 0;
}
