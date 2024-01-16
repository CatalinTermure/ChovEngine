#ifndef CHOVENGINE_INCLUDE_OBJECTS_OBJECT_MANAGER_H_
#define CHOVENGINE_INCLUDE_OBJECTS_OBJECT_MANAGER_H_

#include "objects/game_object.h"
#include "rendering/mesh.h"
#include "scene.h"

#include <filesystem>

#include <absl/container/flat_hash_map.h>

namespace chove::objects {
class ObjectManager {
 public:
  Transform *ImportObject(const std::filesystem::path &path, Transform transform, Scene &scene);
 private:
  absl::flat_hash_map<std::filesystem::path, std::vector<rendering::Mesh>, std::hash<std::filesystem::path>> mesh_cache_;
};

}

#endif //CHOVENGINE_INCLUDE_OBJECTS_OBJECT_MANAGER_H_
