#ifndef CHOVENGINE_INCLUDE_OBJECTS_SCENE_H_
#define CHOVENGINE_INCLUDE_OBJECTS_SCENE_H_

#include <vector>

#include <absl/container/flat_hash_map.h>
#include <entt/entt.hpp>

#include "objects/camera.h"
#include "objects/transform.h"
#include "rendering/mesh.h"

namespace chove::objects {

class GameObject;

class Scene {
 public:
  Scene() = default;
  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;
  Scene(Scene &&) noexcept = default;
  Scene &operator=(Scene &&) noexcept = default;
  ~Scene() = default;

  [[nodiscard]] const Camera &camera() const { return *main_camera_; };
  [[nodiscard]] Camera &camera() { return *main_camera_; };
  void SetMainCamera(Camera &camera) { main_camera_ = &camera; }

  GameObject AddObject(std::vector<rendering::Mesh> &meshes, Transform transform);
  GameObject AddObject(Transform transform);

  template<typename... Components>
  auto GetAllObjectsWith() {
    return registry_.view<Components...>();
  }

  template<typename T>
  void AddComponent(entt::entity entity, T &&component) {
    registry_.emplace<T>(entity, std::forward<decltype(component)>(component));
  }

  template<typename T>
  void RemoveComponentFromAll() {
    registry_.clear<T>();
  }

  [[nodiscard]] entt::registry &registry() { return registry_; }

  [[nodiscard]] bool dirty_bit() const { return dirty_bit_; };
  void ClearDirtyBit() { dirty_bit_ = false; };

 private:
  void SetDirtyBit() { dirty_bit_ = true; };
  bool dirty_bit_{};
  Camera *main_camera_{};
  entt::registry registry_{};
};

}  // namespace chove::objects

#endif  // CHOVENGINE_INCLUDE_OBJECTS_SCENE_H_
