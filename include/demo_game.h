#ifndef LABSEXTRA_INCLUDE_DEMO_GAME_H_
#define LABSEXTRA_INCLUDE_DEMO_GAME_H_

#include "game.h"
#include "objects/object_manager.h"
#include "objects/transform.h"

namespace chove {
class DemoGame : public Game {
 public:
  explicit DemoGame(RendererType renderer_type);

  void Initialize() override;

  void HandleInput() override;
  void HandlePhysics(std::chrono::duration<long long, std::ratio<1, 1'000'000'000>> delta_time) override;
 private:
  glm::vec3 camera_velocity_{};

  objects::Transform *nanosuit;
  objects::Transform *sponza;
  objects::Transform *cube;

  objects::ObjectManager object_manager_{};
};
}  // namespace chove

#endif //LABSEXTRA_INCLUDE_DEMO_GAME_H_
