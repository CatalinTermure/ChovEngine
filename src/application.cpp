#include "application.h"
#include "rendering/opengl/renderer.h"

#include <absl/log/log.h>
#include <thread>

namespace chove {
void Application::SetCurrentScene(std::string scene_name) {
  current_scene_name_ = std::move(scene_name);
  renderer_->SetupScene(current_scene());
}

void Application::Run() {
  physics_time_ = std::chrono::high_resolution_clock::now();
  while (is_running_) {
    auto start_frame_time = std::chrono::high_resolution_clock::now();
    HandleInput();
    auto delta_time = std::chrono::high_resolution_clock::now() - physics_time_;
    HandlePhysics(delta_time);

    physics_time_ += delta_time;
    renderer_->Render();
    auto end_frame_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration target_frame_time = std::chrono::nanoseconds(1'000'000'000 / target_frame_rate_);
    if (end_frame_time - start_frame_time > target_frame_time) {
      LOG(INFO) << std::format("Frame time: {} ms.",
                               duration_cast<std::chrono::milliseconds>(end_frame_time - start_frame_time).count());
    }
  }
}
Application::Application(windowing::RendererType renderer_type) : window_(windowing::Window::Create("Chove",
                                                                                                    {800, 600},
                                                                                                    renderer_type)) {
  renderer_ = std::make_unique<rendering::opengl::Renderer>(&window_);
  target_frame_rate_ = 60;
}
}  // namespace chove::objects
