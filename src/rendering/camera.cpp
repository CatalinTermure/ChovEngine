#include "rendering/camera.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace chove::rendering {

namespace {
bool IsAngleAcceptable(float angle) {
  if (angle < 5.0f || angle > 175.0f) {
    return false;
  }
  return true;
}
}

glm::mat4 Camera::GetTransformMatrix() const {
  return glm::perspective(fov_, aspect_ratio_, near_plane_, far_plane_)
      * glm::lookAt(position_, position_ + look_direction_, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::Move(Camera::Direction direction, float amount) {
  switch (direction) {
    case Direction::eForward:position_ += look_direction_ * amount;
      break;
    case Direction::eBackward:position_ -= look_direction_ * amount;
      break;
    case Direction::eLeft:position_ -= glm::normalize(glm::cross(look_direction_, up_direction)) * amount;
      break;
    case Direction::eRight:position_ += glm::normalize(glm::cross(look_direction_, up_direction)) * amount;
      break;
    case Direction::eUp:position_ += up_direction * amount;
      break;
  }
}

void Camera::Rotate(Camera::RotationDirection direction, float degrees) {
  glm::vec3 new_direction;
  switch (direction) {
    case RotationDirection::eUpward:
      new_direction = glm::normalize(glm::angleAxis(glm::radians(-degrees),
                                                    glm::normalize(glm::cross(look_direction_, up_direction)))
                                         * glm::vec4(look_direction_, 0.0f));
      if (!IsAngleAcceptable(glm::degrees(glm::angle(new_direction, up_direction))))
        return;
      look_direction_ = new_direction;
      break;
    case RotationDirection::eDownward:
      new_direction = glm::normalize(glm::angleAxis(glm::radians(degrees),
                                                    glm::normalize(glm::cross(look_direction_, up_direction)))
                                         * glm::vec4(look_direction_, 0.0f));
      if (!IsAngleAcceptable(glm::degrees(glm::angle(new_direction, up_direction))))
        return;
      look_direction_ = new_direction;
      break;
    case RotationDirection::eLeft:
      look_direction_ = glm::normalize(glm::angleAxis(glm::radians(degrees), up_direction)
                                           * glm::vec4(look_direction_, 0.0f));
      break;
    case RotationDirection::eRight:
      look_direction_ = glm::normalize(glm::angleAxis(glm::radians(-degrees), up_direction)
                                           * glm::vec4(look_direction_, 0.0f));
      break;
  }
}

Camera::Camera(glm::vec4 position,
               glm::vec3 look_direction,
               float fov,
               float aspect_ratio,
               float near_plane,
               float far_plane)
    : position_(position),
      look_direction_(glm::normalize(look_direction)),
      fov_(fov),
      aspect_ratio_(aspect_ratio),
      near_plane_(near_plane),
      far_plane_(far_plane) {}
}
