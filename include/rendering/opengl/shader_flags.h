#ifndef CHOVENGINE_INCLUDE_RENDERING_OPENGL_SHADER_FLAGS_H_
#define CHOVENGINE_INCLUDE_RENDERING_OPENGL_SHADER_FLAGS_H_

#include <cstdint>

enum class ShaderFlagTypes {
  kNoDiffuseTexture = 0,
  kNoAmbientTexture = 1,
  kNoSpecularTexture = 2,
  kNoShininessTexture = 3,
  kNoAlphaTexture = 4,
  kNoBumpTexture = 5,
  kNoDisplacementTexture = 6,
  kPointLightCount = 7,
  kDirectionalLightCount = 8,
  kSpotLightCount = 9,
};

struct ShaderFlag {
  ShaderFlagTypes type;
  int value;

  auto operator<(const ShaderFlag &other) const {
    return type < other.type;
  }
};

#endif //CHOVENGINE_INCLUDE_RENDERING_OPENGL_SHADER_FLAGS_H_
