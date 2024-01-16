#ifndef CHOVENGINE_INCLUDE_OBJECTS_GAME_OBJECT_H_
#define CHOVENGINE_INCLUDE_OBJECTS_GAME_OBJECT_H_

#include "rendering/mesh.h"
#include "objects/transform.h"

namespace chove::objects {

struct GameObject {
  const rendering::Mesh *mesh;
  Transform *transform;
};

} // namespace chove::objects

#endif //CHOVENGINE_INCLUDE_OBJECTS_GAME_OBJECT_H_
