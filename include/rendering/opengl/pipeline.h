#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_PIPELINE_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_PIPELINE_H_

#include "rendering/opengl/shader.h"

namespace chove::rendering::opengl {
class Pipeline {
 public:
  Pipeline(Shader vertex_shader, Shader fragment_shader);
  Pipeline(const Pipeline &) = delete;
  Pipeline &operator=(const Pipeline &) = delete;
  Pipeline(Pipeline &&) noexcept = default;
  Pipeline &operator=(Pipeline &&) noexcept = default;

  Shader vertex_shader;
  Shader fragment_shader;
};
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_PIPELINE_H_
