#ifndef CHOVENGINE_INCLUDE_OBJECTS_OBJECT_MANAGER_H_
#define CHOVENGINE_INCLUDE_OBJECTS_OBJECT_MANAGER_H_

#include <filesystem>
#include <unordered_map>

#include "objects/game_object.h"
#include "objects/scene.h"
#include "rendering/mesh.h"

namespace chove::objects {
class ObjectManager {
 public:
  GameObject ImportObject(const std::filesystem::path &path, Transform transform, Scene &scene);
 private:
  std::unordered_map<std::filesystem::path, std::vector<rendering::Mesh>> mesh_cache_;
};

} // namespace chove::objects

#endif //CHOVENGINE_INCLUDE_OBJECTS_OBJECT_MANAGER_H_
