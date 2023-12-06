#ifndef LABSEXTRA_INCLUDE_OBJECTS_LIGHTS_H_
#define LABSEXTRA_INCLUDE_OBJECTS_LIGHTS_H_

#include <glm/glm.hpp>

namespace chove::objects {

struct DirectionalLight {
  alignas(16) glm::vec3 direction;
  alignas(16) glm::vec3 color;
};

struct PointLight {
  float constant;
  float linear;
  float quadratic;
  alignas(16) glm::vec3 position;
  alignas(16) glm::vec3 color;
};

struct SpotLight {
  float constant;
  float linear;
  float quadratic;
  float inner_cutoff;
  alignas(16) glm::vec3 position;
  float outer_cutoff;
  alignas(16) glm::vec3 direction;
  alignas(16) glm::vec3 color;
};

}

#endif //LABSEXTRA_INCLUDE_OBJECTS_LIGHTS_H_
