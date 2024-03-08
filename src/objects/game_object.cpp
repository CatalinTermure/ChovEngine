#include "objects/game_object.h"

#include "objects/scene.h"

namespace chove::objects {

rendering::Mesh &GameObject::mesh() {
  return GetComponent<rendering::Mesh>();
}

const rendering::Mesh &GameObject::mesh() const {
  return GetComponent<rendering::Mesh>();
}

Transform &GameObject::transform() {
  return GetComponent<Transform>();
}

const Transform &GameObject::transform() const {
  return GetComponent<Transform>();
}

GameObject::GameObject(Scene *scene, entt::entity entity) : entity_(entity), scene_(scene) {}

} // namespace chove::objects
