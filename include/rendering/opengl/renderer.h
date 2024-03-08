#ifndef CHOVENGINE_INCLUDE_RENDERING_OPENGL_RENDERER_H_
#define CHOVENGINE_INCLUDE_RENDERING_OPENGL_RENDERER_H_

#include "rendering/renderer.h"
#include "objects/scene.h"
#include "rendering/opengl/pipeline.h"
#include "rendering/opengl/render_object.h"
#include "rendering/opengl/texture_allocator.h"
#include "rendering/opengl/uniform.h"
#include "windowing/window.h"

#include <memory>

#include <absl/log/log.h>

namespace chove::rendering::opengl {
class Renderer : public rendering::Renderer {
 public:
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;
  Renderer(Renderer &&) noexcept = default;
  Renderer &operator=(Renderer &&) noexcept = default;
  ~Renderer() override = default;

  explicit Renderer(const windowing::Window *window);

  void Render() override;
  void SetupScene(objects::Scene &scene) override;

 private:
  const windowing::Window *window_;
  objects::Scene *scene_;

  std::unique_ptr<TextureAllocator> texture_allocator_;
  std::unique_ptr<ShaderAllocator> shader_allocator_;

  UniformBuffer matrices_ubo_{};
  UniformBuffer lights_{};

  std::vector<Shader> shaders_;
  std::unique_ptr<Shader> depth_map_shader_;
  std::unique_ptr<Texture> white_pixel_;

  UniformBuffer light_space_matrices_{};

  void AttachMaterial(RenderObject &render_object, const Material &material);
  void RenderDepthMap();
};
} // namespace chove::rendering::opengl

#endif //CHOVENGINE_INCLUDE_RENDERING_OPENGL_RENDERER_H_
