#include "camera.h"
namespace chove {

glm::mat4 Camera::GetProjectionMatrix() const {
  return glm::mat4{
      glm::vec4{1.0f / (aspect_ratio_ * glm::tan(fov_ / 2.0f)), 0.0f, 0.0f, 0.0f},
      glm::vec4{0.0f, 1.0f / glm::tan(fov_ / 2.0f), 0.0f, 0.0f},
      glm::vec4{0.0f, 0.0f, (far_plane_ + near_plane_) / (near_plane_ - far_plane_), 1.0f},
      glm::vec4{0.0f, 0.0f, 2.0f * far_plane_ * near_plane_ / (near_plane_ - far_plane_), 0.0f}
  };
}

glm::mat4 Camera::GetViewMatrix() const {
  glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 right = glm::cross(look_direction_, up);
  glm::vec3 up2 = glm::cross(right, look_direction_);
  return {glm::vec4(right, 0.0f),
          glm::vec4(up2, 0.0f),
          glm::vec4(look_direction_, 0.0f),
          glm::vec4(position_)};
}

glm::mat4 Camera::GetTransformMatrix() const {
  return GetProjectionMatrix() * GetViewMatrix();
}
}