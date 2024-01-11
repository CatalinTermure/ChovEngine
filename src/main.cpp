// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <filesystem>
#include <iostream>
#include <fstream>

#include <SDL2/SDL.h>
#include <absl/log/log.h>
#include <absl/log/initialize.h>
#include <absl/log/log_sink.h>
#include <absl/log/log_sink_registry.h>

#include "demo_game.h"

class StdoutLogSink final : public absl::LogSink {
  void Send(const absl::LogEntry &entry) override {
    std::cout << entry.text_message_with_prefix_and_newline();
    log_file_ << entry.text_message_with_prefix_and_newline();
    if (entry.log_severity() == absl::LogSeverity::kError || entry.log_severity() == absl::LogSeverity::kFatal) {
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

  chove::DemoGame game{chove::RendererType::kOpenGL};
  game.Initialize();
  game.Run();

  SDL_Quit();
  return 0;
}
