#ifndef CHOVENGINE_INCLUDE_OBJECTS_GAME_OBJECT_H_
#define CHOVENGINE_INCLUDE_OBJECTS_GAME_OBJECT_H_

#include "objects/scene.h"
#include "objects/transform.h"
#include "rendering/mesh.h"

#include <entt/entt.hpp>

namespace chove::objects {

class GameObject {
 public:
  GameObject() = default;
  GameObject(Scene *scene, entt::entity entity);
  
  template<typename T, typename... Args>
  T &AddComponent(Args &&...args) {
    return scene_->registry().emplace<T>(entity_, std::forward<decltype(args)>(args)...);
  }

  template<typename T>
  [[nodiscard]] const T &GetComponent() const {
    return scene_->registry().get<T>(entity_);
  }

  template<typename T>
  [[nodiscard]] T &GetComponent() {
    return scene_->registry().get<T>(entity_);
  }

  [[nodiscard]] rendering::Mesh &mesh();
  [[nodiscard]] const rendering::Mesh &mesh() const;
  [[nodiscard]] Transform &transform();
  [[nodiscard]] const Transform &transform() const;

  [[nodiscard]] entt::entity entity() const { return entity_; }
 private:
  entt::entity entity_;
  Scene *scene_;
};

} // namespace chove::objects

#endif //CHOVENGINE_INCLUDE_OBJECTS_GAME_OBJECT_H_
