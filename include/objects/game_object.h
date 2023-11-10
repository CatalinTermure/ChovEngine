#ifndef LABSEXTRA_INCLUDE_OBJECTS_GAME_OBJECT_H_
#define LABSEXTRA_INCLUDE_OBJECTS_GAME_OBJECT_H_

#include "rendering/mesh.h"
#include "objects/transform.h"

namespace chove::objects {

struct GameObject {
  const rendering::Mesh *mesh;
  Transform *transform;
};

} // namespace chove::objects

#endif //LABSEXTRA_INCLUDE_OBJECTS_GAME_OBJECT_H_
