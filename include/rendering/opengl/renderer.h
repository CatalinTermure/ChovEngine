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

  explicit Renderer(windowing::Window *window);

  void Render() override;
  void SetupScene(const objects::Scene &scene) override;

 private:
  windowing::Window *window_;
  const objects::Scene *scene_;

  std::unique_ptr<TextureAllocator> texture_allocator_;
  std::unique_ptr<ShaderAllocator> shader_allocator_;

  UniformBuffer matrices_ubo_{};
  UniformBuffer lights_{};

  std::vector<RenderObject> render_objects_;
  std::vector<Shader> shaders_;
  std::unique_ptr<Shader> depth_map_shader_;
  std::vector<Texture> point_depth_maps_;
  std::vector<Texture> directional_depth_maps_;
  std::vector<Texture> spot_depth_maps_;
  std::unique_ptr<Texture> white_pixel_;

  std::vector<GLuint> spot_shadow_framebuffers_;
  std::vector<GLuint> directional_shadow_framebuffers_;
  std::vector<GLuint> point_shadow_framebuffers_;
  UniformBuffer light_space_matrices_{};

  void AttachMaterial(RenderObject &render_object, const Material &material);
};
}

#endif //CHOVENGINE_INCLUDE_RENDERING_OPENGL_RENDERER_H_
