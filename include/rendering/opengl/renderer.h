#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDERER_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDERER_H_

#include "rendering/renderer.h"
#include "rendering/window.h"

namespace chove::rendering::opengl {
class Renderer : public rendering::Renderer {
 public:
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;
  Renderer(Renderer &&) noexcept = default;
  Renderer &operator=(Renderer &&) noexcept = default;

  explicit Renderer(Window *window);

  void Render() override;
  void SetupScene(const objects::Scene &scene) override;

 private:
  Window *window_;
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDERER_H_
