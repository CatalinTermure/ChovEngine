#ifndef LABSEXTRA_INCLUDE_PG_APPLICATION_H_
#define LABSEXTRA_INCLUDE_PG_APPLICATION_H_

#include "application.h"
#include "rendering/window.h"
#include "rendering/mesh.h"

#include <chrono>

namespace chove {

enum class RendererType {
  kOpenGL,
  kVulkan
};

class Game : public Application {
 public:
  explicit Game(RendererType renderer_type);

  void HandleInput() override;
  void HandlePhysics(std::chrono::duration<long long, std::ratio<1, 1'000'000'000>> delta_time) override;
 private:
  rendering::Window window_;
  glm::vec3 camera_velocity_{};

  std::vector<rendering::Mesh> nanosuit;
  std::vector<rendering::Mesh> sponza;
};
}  // namespace chove

#endif //LABSEXTRA_INCLUDE_PG_APPLICATION_H_
