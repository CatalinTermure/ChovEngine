#include "objects/scene.h"

namespace chove::objects {

void Scene::AddObject(const rendering::Mesh &mesh, Transform transform) {
  transforms_.push_back(transform);
  objects_.push_back(GameObject{&mesh, &transforms_.back()});
}
}  // namespace chove::objects
