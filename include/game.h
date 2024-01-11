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

  virtual void Initialize() = 0;
 protected:
  rendering::Window window_;
};
}  // namespace chove

#endif //LABSEXTRA_INCLUDE_PG_APPLICATION_H_
