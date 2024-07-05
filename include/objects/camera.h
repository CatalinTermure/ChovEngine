#ifndef CHOVENGINE_RENDERING_CAMERA_H_
#define CHOVENGINE_RENDERING_CAMERA_H_

#include <glm/glm.hpp>

namespace chove::objects {
class Camera {
 public:
  Camera() = default;

  Camera(
      glm::vec4 position, glm::vec3 look_direction, float fov, float aspect_ratio, float near_plane, float far_plane
  );

  [[nodiscard]] const glm::vec3 &position() const { return position_; }
  [[nodiscard]] const glm::vec3 &look_direction() const { return look_direction_; }
  [[nodiscard]] float near_plane() const { return near_plane_; }
  [[nodiscard]] float far_plane() const { return far_plane_; }

  [[nodiscard]] glm::mat4 GetViewMatrix() const;
  [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

  enum class Direction : uint8_t { eForward, eBackward, eLeft, eRight, eUp };
  enum class RotationDirection : uint8_t { eUpward, eDownward, eLeft, eRight };

  void Move(Direction direction, float amount);
  void Rotate(RotationDirection direction, float degrees);

 private:
  glm::vec3 position_;
  glm::vec3 look_direction_;
  static constexpr auto up_direction = glm::vec3(0.0F, 1.0F, 0.0F);

  float fov_;
  float aspect_ratio_;
  float near_plane_;
  float far_plane_;
};
}  // namespace chove::objects

#endif  // CHOVENGINE_RENDERING_CAMERA_H_
