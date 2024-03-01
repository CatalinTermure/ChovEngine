#include "application.h"
#include "rendering/opengl/renderer.h"
#include "rendering/vulkan/vulkan_renderer.h"

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
    TimePoint start_frame_time = std::chrono::high_resolution_clock::now();
    HandleInput();
    Duration delta_time = std::chrono::high_resolution_clock::now() - physics_time_;
    HandlePhysics(delta_time);

    physics_time_ += delta_time;
    renderer_->Render();
    TimePoint end_frame_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration target_frame_time = std::chrono::nanoseconds(1'000'000'000 / target_frame_rate_);
    if (end_frame_time - start_frame_time > target_frame_time) {
      LOG(INFO) << std::format("Frame time: {} ms.",
                               duration_cast<std::chrono::milliseconds>(end_frame_time - start_frame_time).count());
    }
  }
}
Application::Application(windowing::RendererType renderer_type) : window_(windowing::Window::Create("Chove",
                                                                                                    {1024, 800},
                                                                                                    renderer_type)) {
  if (renderer_type == windowing::RendererType::kOpenGL) {
    renderer_ = std::make_unique<rendering::opengl::Renderer>(&window_);
  } else if (renderer_type == windowing::RendererType::kVulkan) {
    renderer_ = std::make_unique<rendering::vulkan::VulkanRenderer>(rendering::vulkan::VulkanRenderer::Create(window_));
  } else {
    LOG(FATAL) << "Renderer type not supported.";
  }
}
}  // namespace chove
