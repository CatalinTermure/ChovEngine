// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <filesystem>
#include <iostream>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <glm/glm.hpp>
#include <absl/log/log.h>
#include <absl/log/initialize.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>

#include "objects/camera.h"
#include "rendering/renderer.h"
#include "rendering/window.h"
#include "rendering/mesh.h"
#include "objects/scene.h"
#include "objects/lights.h"
#include "rendering/opengl/renderer.h"
#include "game.h"

using chove::objects::Scene;
using chove::objects::Transform;
using chove::objects::Camera;
using chove::objects::SpotLight;
using chove::objects::PointLight;
using chove::rendering::Window;
using chove::rendering::Mesh;
using chove::rendering::Renderer;

class StdoutLogSink final : public absl::LogSink {
  void Send(const absl::LogEntry &entry) override {
    std::cout << entry.text_message_with_prefix_and_newline();
    log_file_ << entry.text_message_with_prefix_and_newline();
    if (entry.log_severity() == absl::LogSeverity::kError) {
      std::cout << entry.stacktrace();
      log_file_ << entry.stacktrace();
    }
  }

 private:
  std::ofstream log_file_{"log.txt"};
};

constexpr int target_frame_rate = 1;
constexpr long long target_frame_time_ns = 1'000'000'000 / target_frame_rate;
constexpr std::chrono::duration target_frame_time = std::chrono::nanoseconds(target_frame_time_ns);

static constexpr const float kCameraSpeed = 1e0F;
constexpr float camera_rotation_speed = 0.1F;

int main() {
  StdoutLogSink log_sink{};
  absl::AddLogSink(&log_sink);
  absl::InitializeLog();

  chove::Game game{chove::RendererType::kOpenGL};
  game.Run();

  SDL_Quit();
  return 0;
}
