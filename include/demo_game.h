#ifndef CHOVENGINE_INCLUDE_DEMO_GAME_H_
#define CHOVENGINE_INCLUDE_DEMO_GAME_H_

#include "application.h"
#include "objects/object_manager.h"
#include "objects/transform.h"

namespace chove {
class DemoGame final : public Application {
 public:
  explicit DemoGame(windowing::RendererType renderer_type);
  DemoGame(const DemoGame &) = delete;
  DemoGame &operator=(const DemoGame &) = delete;
  DemoGame(DemoGame &&) noexcept = delete;
  DemoGame &operator=(DemoGame &&) noexcept = delete;
  ~DemoGame() override = default;

  void HandleInput() override;
  void HandlePhysics(Duration delta_time) override;

 private:
  glm::vec3 camera_velocity_{};
  objects::ObjectManager object_manager_{};
  windowing::WindowPosition last_mouse_position_{};

  objects::GameObject sun_{};
  bool locked_cursor_ = true;
};
}  // namespace chove

#endif  // CHOVENGINE_INCLUDE_DEMO_GAME_H_
