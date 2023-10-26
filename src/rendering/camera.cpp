#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace chove {
glm::mat4 Camera::GetTransformMatrix() {
  look_direction_ = glm::normalize(look_direction_) * 10.0f;
  return glm::perspective(fov_, aspect_ratio_, near_plane_, far_plane_)
      * glm::lookAt(glm::vec3(position_), glm::vec3(position_) + look_direction_, glm::vec3(0.0f, 1.0f, 0.0f));
}
}