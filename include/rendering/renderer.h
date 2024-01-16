#ifndef CHOVENGINE_INCLUDE_RENDERING_RENDERER_H_
#define CHOVENGINE_INCLUDE_RENDERING_RENDERER_H_

#include "objects/scene.h"

namespace chove::rendering {
class Renderer {
 public:
  virtual void SetupScene(const objects::Scene &scene) = 0;
  virtual void Render() = 0;

  virtual ~Renderer() = default;
};
}  // namespace chove::rendering

#endif //CHOVENGINE_INCLUDE_RENDERING_RENDERER_H_
