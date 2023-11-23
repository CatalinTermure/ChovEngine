#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDERER_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDERER_H_

#include "rendering/renderer.h"
#include "rendering/window.h"
#include "objects/scene.h"
#include "rendering/opengl/uniform.h"
#include "rendering/opengl/render_object.h"
#include "rendering/opengl/pipeline.h"

#include <absl/log/log.h>

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
  const objects::Scene *scene_;

  Uniform<glm::mat4> view_;
  Uniform<glm::mat4> projection_;

  std::vector<RenderObject> render_objects_;
  std::vector<Shader> shaders_;

  SDL_GLContext context_;

  void AttachMaterial(RenderObject &render_object, const Material &material);
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDERER_H_
