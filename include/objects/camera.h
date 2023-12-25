#ifndef LABSEXTRA_RENDERING_CAMERA_H_
#define LABSEXTRA_RENDERING_CAMERA_H_

#include <glm/glm.hpp>

#include <cmath>

namespace chove::objects {
class Camera {
 public:
  Camera() = default;

  Camera(glm::vec4 position,
         glm::vec3 look_direction,
         float fov,
         float aspect_ratio,
         float near_plane,
         float far_plane);

  [[nodiscard]] const glm::vec3 &position() const { return position_; }
  [[nodiscard]] const glm::vec3 &look_direction() const { return look_direction_; }
  [[nodiscard]] float near_plane() const { return near_plane_; }
  [[nodiscard]] float far_plane() const { return far_plane_; }

  [[nodiscard]] glm::mat4 GetViewMatrix() const;
  [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

  enum class Direction {
    eForward,
    eBackward,
    eLeft,
    eRight,
    eUp
  };

  enum class RotationDirection {
    eUpward,
    eDownward,
    eLeft,
    eRight
  };

  void Move(Direction direction, float amount);
  void Rotate(RotationDirection direction, float degrees);

 private:
  glm::vec3 position_;
  glm::vec3 look_direction_;
  static constexpr glm::vec3 up_direction = glm::vec3(0.0f, 1.0f, 0.0f);

  float fov_;
  float aspect_ratio_;
  float near_plane_;
  float far_plane_;
};
}

#endif //LABSEXTRA_RENDERING_CAMERA_H_
