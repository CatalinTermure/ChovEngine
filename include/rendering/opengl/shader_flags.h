#ifndef LABSEXTRA_INCLUDE_RENDERING_OPENGL_SHADER_FLAGS_H_
#define LABSEXTRA_INCLUDE_RENDERING_OPENGL_SHADER_FLAGS_H_

#include <cstdint>

enum class ShaderFlags : uint64_t {
  kNoDiffuseTexture = 1 << 0,
  kNoAmbientTexture = 1 << 1,
  kNoSpecularTexture = 1 << 2,
  kNoShininessTexture = 1 << 3,
  kNoAlphaTexture = 1 << 4,
  kNoBumpTexture = 1 << 5,
  kNoDisplacementTexture = 1 << 6,
};

inline ShaderFlags operator|(ShaderFlags lhs, ShaderFlags rhs) {
  return static_cast<ShaderFlags>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs));
}

inline ShaderFlags operator&(ShaderFlags lhs, ShaderFlags rhs) {
  return static_cast<ShaderFlags>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs));
}

inline ShaderFlags &operator|=(ShaderFlags &lhs, ShaderFlags rhs) {
  lhs = lhs | rhs;
  return lhs;
}

#endif //LABSEXTRA_INCLUDE_RENDERING_OPENGL_SHADER_FLAGS_H_
