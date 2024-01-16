#ifndef CHOVENGINE_INCLUDE_RENDERING_MATERIAL_H_
#define CHOVENGINE_INCLUDE_RENDERING_MATERIAL_H_

#include <glm/glm.hpp>
#include <memory>
#include <filesystem>

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
  float shininess;
  float optical_density;
  float dissolve;
  glm::vec3 transmission_filter_color;
  glm::vec3 ambient_color;
  glm::vec3 diffuse_color;
  glm::vec3 specular_color;
  std::optional<std::filesystem::path> ambient_texture;
  std::optional<std::filesystem::path> diffuse_texture;
  std::optional<std::filesystem::path> specular_texture;
  std::optional<std::filesystem::path> shininess_texture;
  std::optional<std::filesystem::path> alpha_texture;
  std::optional<std::filesystem::path> bump_texture;
  std::optional<std::filesystem::path> displacement_texture;
  IllumType illumination_model;
};
} // namespace chove::rendering

#endif //CHOVENGINE_INCLUDE_RENDERING_MATERIAL_H_
