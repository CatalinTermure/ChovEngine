#ifndef LABSEXTRA_INCLUDE_OBJECTS_TRANSFORM_H_
#define LABSEXTRA_INCLUDE_OBJECTS_TRANSFORM_H_

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace chove::objects {

struct Transform {
  glm::vec3 location;
  glm::quat rotation;
  glm::vec3 scale;

  // While not necessary for the transform matrix, we also store the velocity and angular velocity of the object so that
  // the structure is more cache friendly.

  glm::vec3 velocity;
  glm::quat angular_velocity;

  [[nodiscard]] glm::mat4 GetMatrix() const {
    return glm::translate(glm::scale(glm::toMat4(rotation), scale), location);
  }
};

}  // namespace chove::objects

#endif //LABSEXTRA_INCLUDE_OBJECTS_TRANSFORM_H_
