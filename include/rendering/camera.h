#ifndef LABSEXTRA_RENDERING_CAMERA_H_
#define LABSEXTRA_RENDERING_CAMERA_H_

#include <glm/glm.hpp>

namespace chove {

class Camera {
 public:
  constexpr Camera(glm::vec4 position, glm::vec3 look_direction, float fov, float aspect_ratio, float near_plane, float far_plane)
      : position_(position),
        look_direction_(look_direction),
        fov_(fov),
        aspect_ratio_(aspect_ratio),
        near_plane_(near_plane),
        far_plane_(far_plane) {}

  [[nodiscard]] glm::vec4 position() const { return position_; }
  [[nodiscard]] glm::vec3 look_direction() const { return look_direction_; }

  [[nodiscard]] glm::mat4 GetTransformMatrix() const;
 private:
  glm::vec4 position_;
  glm::vec3 look_direction_;

  float fov_;
  float aspect_ratio_;
  float near_plane_;
  float far_plane_;

  [[nodiscard]] glm::mat4 GetViewMatrix() const;

  [[nodiscard]] glm::mat4 GetProjectionMatrix() const;
};
}

#endif //LABSEXTRA_RENDERING_CAMERA_H_
