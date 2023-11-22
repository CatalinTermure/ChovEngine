// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <filesystem>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <glm/glm.hpp>
#include <absl/log/log.h>
#include <absl/log/initialize.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>

#include "rendering/camera.h"
#include "rendering/renderer.h"
#include "rendering/window.h"
#include "rendering/mesh.h"
#include "objects/scene.h"
#include "rendering/opengl/renderer.h"

using chove::objects::Scene;
using chove::objects::Transform;
using chove::rendering::Camera;
using chove::rendering::Window;
using chove::rendering::Mesh;
using chove::rendering::Renderer;

class StdoutLogSink final : public absl::LogSink {
  void Send(const absl::LogEntry &entry) override {
    std::cout << entry.text_message_with_prefix_and_newline();
  }
};

constexpr int target_frame_rate = 60;
constexpr long long target_frame_time_ns = 1'000'000'000 / target_frame_rate;
constexpr std::chrono::duration target_frame_time = std::chrono::nanoseconds(target_frame_time_ns);

static constexpr const float kCameraSpeed = 1.0F;
constexpr float camera_rotation_speed = 0.1F;

int main() {
  StdoutLogSink log_sink{};
  absl::AddLogSink(&log_sink);
  absl::InitializeLog();

  Window window;

  std::vector<Mesh> meshes = Mesh::ImportFromObj(std::filesystem::current_path() / "models" / "cube.obj");

  std::unique_ptr<Renderer> renderer = std::make_unique<chove::rendering::opengl::Renderer>(&window);

  Scene scene;
  scene.camera() = Camera(
      {0.0F, 0.0F, -1.0F, 1.0F},
      {0.0F, 0.0F, 1.0F},
      glm::radians(55.0F),
      static_cast<float>(window.width()) / static_cast<float>(window.height()),
      0.1F,
      100000.0F);
  glm::vec3 camera_velocity = {0.0F, 0.0F, 0.0F};
  for (auto &mesh : meshes) {
    for (auto &vertex : mesh.vertices) {
      vertex.position *= -1;
    }
    scene.AddObject(mesh, {glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::identity<glm::quat>(),
                           glm::vec3(1.0f, 1.0f, 1.0f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::identity<glm::quat>()});
  }

  renderer->SetupScene(scene);

  bool should_app_close = false;
  auto time = std::chrono::high_resolution_clock::now();
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
          case SDLK_w:camera_velocity.y = kCameraSpeed;
            break;
          case SDLK_a:camera_velocity.x = -kCameraSpeed;
            break;
          case SDLK_s:camera_velocity.y = -kCameraSpeed;
            break;
          case SDLK_d:camera_velocity.x = kCameraSpeed;
            break;
          case SDLK_SPACE:camera_velocity.z = -kCameraSpeed;
            break;
          case SDLK_LSHIFT:camera_velocity.z = kCameraSpeed;
            break;
          case SDLK_r:
            scene.objects()[0].transform->rotation *= glm::angleAxis(glm::radians(10.0F), glm::vec3(0.0F, 1.0F, 0.0F));
            break;
          default:break;
        }
      } else if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
          case SDLK_s:
          case SDLK_w:camera_velocity.y = 0.0F;
            break;
          case SDLK_d:
          case SDLK_a:camera_velocity.x = 0.0F;
            break;
          case SDLK_SPACE:
          case SDLK_LSHIFT:camera_velocity.z = 0.0F;
            break;
          default: LOG(INFO) << std::format("Camera position is: ({},{},{})",
                                            scene.camera().position().x,
                                            scene.camera().position().y,
                                            scene.camera().position().z);
            LOG(INFO) << std::format("Camera look direction is: ({},{},{})",
                                     scene.camera().look_direction().x,
                                     scene.camera().look_direction().y,
                                     scene.camera().look_direction().z);

            break;
        }
      } else if (event.type == SDL_MOUSEMOTION) {
        scene.camera().Rotate(Camera::RotationDirection::eRight,
                              camera_rotation_speed * static_cast<float>(event.motion.xrel));
        scene.camera().Rotate(Camera::RotationDirection::eDownward,
                              camera_rotation_speed * static_cast<float>(event.motion.yrel));
      }
    }

    auto deltaTime = std::chrono::high_resolution_clock::now() - time;

    scene.camera().Move(Camera::Direction::eForward,
                        camera_velocity.y * static_cast<float>(std::chrono::nanoseconds(deltaTime).count()) / 4e7f);
    scene.camera().Move(Camera::Direction::eRight,
                        camera_velocity.x * static_cast<float>(std::chrono::nanoseconds(deltaTime).count()) / 4e7f);
    scene.camera().Move(Camera::Direction::eUp,
                        camera_velocity.z * static_cast<float>(std::chrono::nanoseconds(deltaTime).count()) / 4e7f);

    time += deltaTime;

    renderer->Render();

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
