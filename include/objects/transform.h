#ifndef CHOVENGINE_INCLUDE_OBJECTS_TRANSFORM_H_
#define CHOVENGINE_INCLUDE_OBJECTS_TRANSFORM_H_

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace chove::objects {

struct Transform {
  Transform() = default;
  Transform(glm::vec3 location, glm::quat rotation, glm::vec3 scale, Transform *parent = nullptr)
      : location(location),
        rotation(rotation),
        scale(scale),
        parent(parent),
        velocity(glm::vec3(0.0f)),
        angular_velocity(glm::identity<glm::quat>()) {}

  glm::vec3 location{};
  glm::quat rotation{};
  glm::vec3 scale{};
  Transform *parent = nullptr;

  // While not necessary for the transform matrix, we also store the velocity and angular velocity of the object so that
  // the structure is more cache friendly.

  glm::vec3 velocity{};
  glm::quat angular_velocity{};

  [[nodiscard]] glm::mat4 GetMatrix() const {
    glm::mat4 current_matrix =
        glm::translate(glm::identity<glm::mat4>(), location) * glm::scale(glm::identity<glm::mat4>(), scale)
            * glm::toMat4(rotation);
    if (parent == nullptr) return current_matrix;
    return parent->GetMatrix() * current_matrix;
  }
};

}  // namespace chove::objects

#endif //CHOVENGINE_INCLUDE_OBJECTS_TRANSFORM_H_
