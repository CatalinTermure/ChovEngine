#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDERER_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDERER_H_

#include "rendering/renderer.h"
#include "rendering/window.h"
#include "objects/scene.h"
#include "rendering/opengl/uniform.h"
#include "rendering/opengl/render_object.h"
#include "rendering/opengl/pipeline.h"
#include "rendering/opengl/texture_allocator.h"

#include <memory>

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

  SDL_GLContext context_;

  std::unique_ptr<TextureAllocator> texture_allocator_;
  std::unique_ptr<ShaderAllocator> shader_allocator_;

  UniformBuffer view_projection_matrices_{};
  UniformBuffer lights_{};

  std::vector<RenderObject> render_objects_;
  std::vector<Shader> shaders_;
  std::vector<Shader> shadow_shaders_;
  std::vector<Texture> depth_maps_;
  std::vector<GLuint> shadow_framebuffers_;
  UniformBuffer light_space_matrices_{};

  void AttachMaterial(RenderObject &render_object, const Material &material);
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_RENDERER_H_
