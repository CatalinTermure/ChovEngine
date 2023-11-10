#ifndef LABSEXTRA_INCLUDE_OBJECTS_GAME_OBJECT_BATCH_H_
#define LABSEXTRA_INCLUDE_OBJECTS_GAME_OBJECT_BATCH_H_

#include <vector>

#include "game_object.h"

namespace chove::objects {
class GameObjectBatch {
 public:
  [[nodiscard]] const std::vector<GameObject> &objects() const { return objects_; }
 private:
  std::vector<GameObject> objects_;
};
} // namespace chove::objects

#endif //LABSEXTRA_INCLUDE_OBJECTS_GAME_OBJECT_BATCH_H_
