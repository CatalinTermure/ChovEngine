#ifndef LABSEXTRA_INCLUDE_RENDERING_MATERIAL_H_
#define LABSEXTRA_INCLUDE_RENDERING_MATERIAL_H_

#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

#include "texture.h"

namespace chove::rendering {
enum class IllumType {
  eColorNoAmbient = 0,
  eColorAmbient = 1,
  eHighlight = 2,
  eReflectionRayTrace = 3,
  eGlassRayTrace = 4,
  eFresnelRayTrace = 5,
  eRefractionRayTrace = 6,
  eRefractionFresnelRayTrace = 7,
  eReflection = 8,
  eGlass = 9,
  eShadowOnInvisibleSurfaces = 10
};

struct Material {
  float specular_exponent;
  float optical_density;
  float alpha;
  glm::vec3 transmission_filter_color;
  glm::vec3 ambient_color;
  glm::vec3 diffuse_color;
  glm::vec3 specular_color;
  std::shared_ptr<Texture> ambient_texture;
  std::shared_ptr<Texture> diffuse_texture;
  std::shared_ptr<Texture> specular_texture;
  std::shared_ptr<Texture> specular_exponent_texture;
  std::shared_ptr<Texture> alpha_texture;
  std::shared_ptr<Texture> bump_texture;
  std::shared_ptr<Texture> displacement_texture;
  IllumType illumination_model;
};
} // namespace chove::rendering

#endif //LABSEXTRA_INCLUDE_RENDERING_MATERIAL_H_
