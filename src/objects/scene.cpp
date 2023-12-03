#include "objects/scene.h"

namespace chove::objects {

void Scene::AddObject(const rendering::Mesh &mesh, Transform transform) {
  transforms_.push_back(transform);
  objects_.push_back(GameObject{&mesh, &transforms_.back()});
}

// TODO: fix this hack, needed for the pointers to be stable(up to 10k objects), will need to use indices into the vector,
// but that implementation will need an "Application" class or some sort of global context to not pass around and store scene references everywhere.
Scene::Scene() {
  transforms_.reserve(10000);
}

void Scene::AddLight(PointLight light) {
  point_lights_.push_back(light);
}

void Scene::AddLight(SpotLight light) {
  spot_lights_.push_back(light);
}
}  // namespace chove::objects
