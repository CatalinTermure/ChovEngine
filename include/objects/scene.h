#ifndef LABSEXTRA_INCLUDE_OBJECTS_SCENE_H_
#define LABSEXTRA_INCLUDE_OBJECTS_SCENE_H_

#include <vector>

#include "game_object.h"
#include "rendering/camera.h"

namespace chove::objects {

class Scene {
 public:
  Scene();
  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;
  Scene(Scene &&) noexcept = default;
  Scene &operator=(Scene &&) noexcept = default;

  [[nodiscard]] const std::vector<GameObject> &objects() const { return objects_; };
  [[nodiscard]] rendering::Camera &camera() { return camera_; };
  [[nodiscard]] const rendering::Camera &camera() const { return camera_; };

  void AddObject(const rendering::Mesh &mesh, Transform transform);
 private:
  std::vector<GameObject> objects_{};
  rendering::Camera camera_{};

  // store all transforms here for cache coherency
  std::vector<Transform> transforms_{};
};

} // namespace chove::objects

#endif //LABSEXTRA_INCLUDE_OBJECTS_SCENE_H_
