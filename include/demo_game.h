#ifndef CHOVENGINE_INCLUDE_DEMO_GAME_H_
#define CHOVENGINE_INCLUDE_DEMO_GAME_H_

#include "objects/object_manager.h"
#include "application.h"
#include "objects/transform.h"

namespace chove {
class DemoGame : public Application {
 public:
  explicit DemoGame(windowing::RendererType renderer_type);

  void Initialize() override;

  void HandleInput() override;
  void HandlePhysics(std::chrono::duration<long long, std::ratio<1, 1'000'000'000>> delta_time) override;
 private:
  glm::vec3 camera_velocity_{};

  objects::Transform *nanosuit{};
  objects::Transform *sponza{};
  objects::Transform *cube{};

  objects::ObjectManager object_manager_{};

  windowing::WindowPosition last_mouse_position_{};
};
}  // namespace chove

#endif //CHOVENGINE_INCLUDE_DEMO_GAME_H_
