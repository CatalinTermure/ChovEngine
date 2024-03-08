#include "objects/scene.h"

#include "objects/game_object.h"
#include "rendering/mesh.h"

namespace chove::objects {

using rendering::Mesh;

GameObject Scene::AddObject(std::vector<rendering::Mesh> &meshes, Transform transform) {
  GameObject game_object{this, registry_.create()};
  Transform *parent = &game_object.AddComponent<Transform>(transform);
  for (auto &mesh : meshes) {
    GameObject sub_object{this, registry_.create()};
    sub_object.AddComponent<Transform>(glm::vec3(0.0F), parent);
    sub_object.AddComponent<Mesh *>(&mesh);
  }
  SetDirtyBit();
  return game_object;
}

GameObject Scene::AddObject(Transform transform) {
  GameObject game_object{this, registry_.create()};
  game_object.AddComponent<Transform>(transform);
  SetDirtyBit();
  return game_object;
}

}  // namespace chove::objects
