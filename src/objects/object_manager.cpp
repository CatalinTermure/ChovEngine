#include "objects/object_manager.h"

namespace chove::objects {

GameObject ObjectManager::ImportObject(const std::filesystem::path &path, Transform transform, Scene &scene) {
  if (!mesh_cache_.contains(path)) {
    mesh_cache_[path] = rendering::Mesh::ImportFromObj(path);
  }
  return scene.AddObject(mesh_cache_[path], transform);
}
}  // namespace chove::objects
