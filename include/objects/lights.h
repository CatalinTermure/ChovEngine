#ifndef LABSEXTRA_INCLUDE_OBJECTS_LIGHTS_H_
#define LABSEXTRA_INCLUDE_OBJECTS_LIGHTS_H_

#include <glm/glm.hpp>

namespace chove::objects {

struct DirectionalLight {
  alignas(16) glm::vec3 direction;
  float ambient;
  alignas(16) glm::vec3 color;
};

struct PointLight {
  float constant;
  float linear;
  float quadratic;
  float near_plane;
  alignas(16) glm::vec3 position;
  float far_plane;
  alignas(16) glm::vec3 color;
  float ambient;
  alignas(16) glm::vec3 positionEyeSpace;
};

struct SpotLight {
  float constant;
  float linear;
  float quadratic;
  float inner_cutoff;
  alignas(16) glm::vec3 position;
  float outer_cutoff;
  alignas(16) glm::vec3 direction;
  float ambient;
  alignas(16) glm::vec3 color;
};

}

#endif //LABSEXTRA_INCLUDE_OBJECTS_LIGHTS_H_
