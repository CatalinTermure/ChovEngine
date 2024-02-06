#ifndef CHOVENGINE_INCLUDE_OBJECTS_APPLICATION_H_
#define CHOVENGINE_INCLUDE_OBJECTS_APPLICATION_H_

#include "objects/scene.h"
#include "rendering/renderer.h"
#include "windowing/window.h"

#include <absl/container/flat_hash_map.h>

#include <chrono>

namespace chove {

class Application {
 public:
  explicit Application(windowing::RendererType renderer_type);
  void Run();

  virtual void Initialize() = 0;
  void SetCurrentScene(std::string scene_name);

  [[nodiscard]] objects::Scene &current_scene() { return scenes_.at(current_scene_name_); }
  [[nodiscard]] const objects::Scene &current_scene() const { return scenes_.at(current_scene_name_); }

 protected:
  virtual void HandleInput() = 0;
  virtual void HandlePhysics(std::chrono::duration<long long, std::ratio<1, 1'000'000'000>> delta_time) = 0;

  absl::flat_hash_map<std::string, objects::Scene> scenes_;
  std::string current_scene_name_;

  std::unique_ptr<rendering::Renderer> renderer_;
  windowing::Window window_;

  std::chrono::time_point<std::chrono::high_resolution_clock> physics_time_;

  bool is_running_;
  int target_frame_rate_;

};
}  // namespace chove

#endif //CHOVENGINE_INCLUDE_OBJECTS_APPLICATION_H_
