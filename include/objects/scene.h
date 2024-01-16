#ifndef CHOVENGINE_INCLUDE_OBJECTS_SCENE_H_
#define CHOVENGINE_INCLUDE_OBJECTS_SCENE_H_

#include <vector>
#include <absl/container/flat_hash_map.h>

#include "game_object.h"
#include "camera.h"
#include "lights.h"

namespace chove::objects {

class Scene {
 public:
  Scene();
  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;
  Scene(Scene &&) noexcept = default;
  Scene &operator=(Scene &&) noexcept = default;

  [[nodiscard]] const std::vector<GameObject> &objects() const { return objects_; };
  [[nodiscard]] Camera &camera() { return camera_; };
  [[nodiscard]] const Camera &camera() const { return camera_; };

  void AddObject(const rendering::Mesh &mesh, Transform transform);
  Transform *AddObject(const std::vector<rendering::Mesh> &meshes, Transform transform);
  void SetDirectionalLight(DirectionalLight light);
  void AddLight(PointLight light);
  void AddLight(SpotLight light);

  [[nodiscard]] const DirectionalLight &directional_light() const { return directional_light_; };
  [[nodiscard]] const std::vector<PointLight> &point_lights() const { return point_lights_; };
  [[nodiscard]] const std::vector<SpotLight> &spot_lights() const { return spot_lights_; };

  [[nodiscard]] bool dirty_bit() const { return dirty_bit_; };
  void ClearDirtyBit() { dirty_bit_ = false; };
 private:
  void SetDirtyBit() { dirty_bit_ = true; };

  std::vector<GameObject> objects_{};
  Camera camera_{};

  // store all transforms here for cache coherency
  std::vector<Transform> transforms_{};
  absl::flat_hash_map<Transform *, std::vector<Transform *>> children_{};

  DirectionalLight directional_light_{};
  std::vector<PointLight> point_lights_{};
  std::vector<SpotLight> spot_lights_{};

  bool dirty_bit_{};
};

} // namespace chove::objects

#endif //CHOVENGINE_INCLUDE_OBJECTS_SCENE_H_
