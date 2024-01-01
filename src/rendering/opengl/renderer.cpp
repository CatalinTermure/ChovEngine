#include "rendering/opengl/renderer.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_inverse.hpp>

namespace chove::rendering::opengl {
namespace {
void GLAPIENTRY MessageCallback(GLenum source,
                                GLenum type,
                                unsigned int id,
                                GLenum severity,
                                [[maybe_unused]] GLsizei length,
                                const char *message,
                                [[maybe_unused]] const void *userParam) {
  // ignore non-significant error/warning codes
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

  std::string msg = "---------------\n";
  msg += std::format("Debug message ({}): {}\n", id, message);

  switch (source) {
    case GL_DEBUG_SOURCE_API: msg += "Source: API";
      break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: msg += "Source: Window System";
      break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: msg += "Source: Shader Compiler";
      break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: msg += "Source: Third Party";
      break;
    case GL_DEBUG_SOURCE_APPLICATION: msg += "Source: Application";
      break;
    case GL_DEBUG_SOURCE_OTHER: msg += "Source: Other";
      break;
    default: msg += "Source: Unknown";
      break;
  }
  msg += "\n";

  switch (type) {
    case GL_DEBUG_TYPE_ERROR: msg += "Type: Error";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: msg += "Type: Deprecated Behaviour";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: msg += "Type: Undefined Behaviour";
      break;
    case GL_DEBUG_TYPE_PORTABILITY: msg += "Type: Portability";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE: msg += "Type: Performance";
      break;
    case GL_DEBUG_TYPE_MARKER: msg += "Type: Marker";
      break;
    case GL_DEBUG_TYPE_PUSH_GROUP: msg += "Type: Push Group";
      break;
    case GL_DEBUG_TYPE_POP_GROUP: msg += "Type: Pop Group";
      break;
    case GL_DEBUG_TYPE_OTHER: msg += "Type: Other";
      break;
    default: msg += "Type: Unknown";
      break;
  }
  msg += "\n";

  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: msg += "Severity: high";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM: msg += "Severity: medium";
      break;
    case GL_DEBUG_SEVERITY_LOW: msg += "Severity: low";
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: msg += "Severity: notification";
      break;
    default: msg += "Severity: unknown";
      break;
  }
  msg += "\n-------------------------------------------------------------";
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: LOG(ERROR) << msg;
      break;
    case GL_DEBUG_SEVERITY_MEDIUM: LOG(WARNING) << msg;
      break;
    case GL_DEBUG_SEVERITY_LOW: LOG(INFO) << msg;
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: LOG(INFO) << msg;
      break;
    default: LOG(INFO) << msg;
      break;
  }
}

struct MatricesUBOData {
  [[maybe_unused]] alignas(16) glm::mat4 view;
  [[maybe_unused]] alignas(16) glm::mat4 projection;
};

struct MaterialUBOData {
  [[maybe_unused]] float shininess;
  [[maybe_unused]] float opticalDensity;
  [[maybe_unused]] float dissolve;
  [[maybe_unused]] alignas(16) glm::vec3 diffuseColor;
  [[maybe_unused]] alignas(16) glm::vec3 ambientColor;
  [[maybe_unused]] alignas(16) glm::vec3 specularColor;
  [[maybe_unused]] alignas(16) glm::vec3 transmissionFilterColor;
};

constexpr int kMatricesUBOBindingPoint = 0;
constexpr int kMaterialUBOBindingPoint = 1;
constexpr int kLightsUBOBindingPoint = 2;
constexpr int kLightSpaceMatricesUBOBindingPoint = 3;

constexpr int kShadowMapSize = 2048;

constexpr glm::vec3 kCubeMapDirections[6] = {
    glm::vec3(1.0F, 0.0F, 0.0F),
    glm::vec3(-1.0F, 0.0F, 0.0F),
    glm::vec3(0.0F, 1.0F, 0.0F),
    glm::vec3(0.0F, -1.0F, 0.0F),
    glm::vec3(0.0F, 0.0F, 1.0F),
    glm::vec3(0.0F, 0.0F, -1.0F)
};

constexpr glm::vec3 kCubeMapUpVectors[6] = {
    glm::vec3(0.0F, -1.0F, 0.0F),
    glm::vec3(0.0F, -1.0F, 0.0F),
    glm::vec3(0.0F, 0.0F, 1.0F),
    glm::vec3(0.0F, 0.0F, -1.0F),
    glm::vec3(0.0F, -1.0F, 0.0F),
    glm::vec3(0.0F, -1.0F, 0.0F)
};

}

using objects::PointLight;
using objects::DirectionalLight;
using objects::SpotLight;
using objects::Scene;
using objects::GameObject;

Renderer::Renderer(Window *window) : window_(window), scene_(nullptr) {
  context_ = SDL_GL_CreateContext(*window_);

  glewExperimental = GL_TRUE;
  glewInit();

  glClearColor(0.3f, 0.3f, 0.3f, 1.0);
  glViewport(0, 0, window_->width(), window_->height());

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glEnable(GL_FRAMEBUFFER_SRGB);

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, nullptr);

  texture_allocator_ = std::make_unique<TextureAllocator>();
  shader_allocator_ = std::make_unique<ShaderAllocator>();
}

void Renderer::Render() {
  if (scene_ == nullptr) return;

  if (scene_->dirty_bit()) {
    SetupScene(*scene_);
  }

  // Start depth map render pass

  glViewport(0, 0, kShadowMapSize, kShadowMapSize);

  point_shadow_shader_->Use();
  for (int i = 0; i < scene_->point_lights().size(); ++i) {
    glBindFramebuffer(GL_FRAMEBUFFER, point_shadow_framebuffers_[i]);
    glm::mat4 light_projection = glm::perspective(glm::radians(90.0F),
                                                  1.0F,
                                                  scene_->point_lights()[i].near_plane,
                                                  scene_->point_lights()[i].far_plane);
    for (int j = 0; j < 6; j++) {
      glFramebufferTexture2D(GL_FRAMEBUFFER,
                             GL_DEPTH_ATTACHMENT,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + j,
                             point_depth_maps_[i].texture(),
                             0);
      glClear(GL_DEPTH_BUFFER_BIT);

      glm::mat4 light_view = glm::lookAt(scene_->point_lights()[i].position,
                                         scene_->point_lights()[i].position + kCubeMapDirections[j],
                                         kCubeMapUpVectors[j]);
      glm::mat4 light_space_matrix = light_projection * light_view;
      Uniform<glm::mat4> light_space_matrix_uniform(point_shadow_shader_->program(),
                                                    "lightSpaceMatrix",
                                                    light_space_matrix);
      light_space_matrices_.UpdateSubData(glm::value_ptr(light_space_matrix), i * sizeof(glm::mat4), sizeof(glm::mat4));
      light_space_matrices_.Rebind();
      light_space_matrix_uniform.UpdateValue(light_space_matrix);
      for (RenderObject &render_object : render_objects_) {
        render_object.shadow_model.UpdateValue(scene_->objects()[render_object.object_index].transform->GetMatrix());
        glBindVertexArray(render_object.vao);
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(scene_->objects()[render_object.object_index].mesh->indices.size()),
                       GL_UNSIGNED_INT,
                       nullptr);
        glBindVertexArray(0);
      }
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Start actual rendering pass

  glViewport(0, 0, window_->width(), window_->height());

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  MatricesUBOData matrices_ubo_data = {scene_->camera().GetViewMatrix(),
                                       scene_->camera().GetProjectionMatrix()};
  matrices_ubo_.UpdateData(&matrices_ubo_data, sizeof(MatricesUBOData));
  matrices_ubo_.Rebind();

  // Send light data

  {
    DirectionalLight light = scene_->directional_light();
    light.direction = glm::vec3(scene_->camera().GetViewMatrix() * glm::vec4(light.direction, 0.0F));
    lights_.UpdateSubData(&light, 0, sizeof(DirectionalLight));
  }

  {
    for (int i = 0; i < scene_->point_lights().size(); i++) {
      PointLight light = scene_->point_lights()[i];
      light.positionEyeSpace = glm::vec3(scene_->camera().GetViewMatrix() * glm::vec4(light.position, 1.0F));
      lights_.UpdateSubData(&light, sizeof(DirectionalLight) + i * sizeof(PointLight), sizeof(PointLight));
    }
  }
  lights_.Rebind();

  // Send object data

  MaterialUBOData material_ubo_data{};

  for (RenderObject &render_object : render_objects_) {
    shaders_[render_object.shader_index].Use();

    render_object.model.UpdateValue(scene_->objects()[render_object.object_index].transform->GetMatrix());
    render_object.normal_matrix.UpdateValue(
        glm::mat3(glm::inverseTranspose(
            matrices_ubo_data.view * scene_->objects()[render_object.object_index].transform->GetMatrix())));

    const Material &material = scene_->objects()[render_object.object_index].mesh->material;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
    material_ubo_data.shininess = material.shininess;
    material_ubo_data.opticalDensity = material.optical_density;
    material_ubo_data.dissolve = material.dissolve;
    material_ubo_data.diffuseColor = material.diffuse_color;
    material_ubo_data.ambientColor = material.ambient_color;
    material_ubo_data.specularColor = material.specular_color;
    material_ubo_data.transmissionFilterColor = material.transmission_filter_color;
#pragma clang diagnostic pop

    render_object.material_data.UpdateData(&material_ubo_data, sizeof(MaterialUBOData));
    render_object.material_data.Rebind();

    // Send shadow map data

    int texture_index = 0;
    for (int i = 0; i < scene_->point_lights().size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glUniform1i(glGetUniformLocation(shaders_[render_object.shader_index].program(),
                                       std::format("{}[{}]", point_depth_maps_[i].name(), i).c_str()),
                  i);
      glBindTexture(GL_TEXTURE_CUBE_MAP, point_depth_maps_[i].texture());
      texture_index++;
    }

    for (int i = 0; i < render_object.textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + texture_index + i);
      glBindTexture(GL_TEXTURE_2D, render_object.textures[i].texture());
      int location = glGetUniformLocation(shaders_[render_object.shader_index].program(),
                                          render_object.textures[i].name().c_str());
      glUniform1i(location, texture_index + i);
    }

    glBindVertexArray(render_object.vao);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(scene_->objects()[render_object.object_index].mesh->indices.size()),
                   GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);

    for (int i = 0; i < render_object.textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + texture_index + i);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }

  SDL_GL_SwapWindow(*window_);
}

void Renderer::SetupScene(const Scene &scene) {
  LOG(INFO) << "Starting setup scene";
  scene_ = &scene;
  // this const cast is fine, we are not modifying the scene, just clearing the dirty bit
  const_cast<Scene *>(scene_)->ClearDirtyBit();
  LOG(INFO) << "Setting up shaders";

  LOG(INFO) << "Setting up uniforms";

  render_objects_.clear();
  shaders_.clear();
  point_shadow_shader_.reset();
  point_depth_maps_.clear();
  point_shadow_framebuffers_.clear();

  render_objects_.reserve(scene.objects().size());
  shaders_.reserve(scene.objects().size());

  matrices_ubo_ = UniformBuffer(2 * sizeof(glm::mat4));
  light_space_matrices_ = UniformBuffer(scene_->point_lights().size() * sizeof(glm::mat4));
  lights_ = UniformBuffer(
      sizeof(DirectionalLight) + scene_->point_lights().size() * sizeof(PointLight)
          + scene_->spot_lights().size() * sizeof(SpotLight));

  point_shadow_shader_ = std::make_unique<Shader>("shaders/point_depth_map.vert",
                                                  std::vector<ShaderFlag>{},
                                                  "shaders/point_depth_map.frag",
                                                  std::vector<ShaderFlag>{},
                                                  *shader_allocator_);

  point_shadow_framebuffers_.resize(scene_->point_lights().size());
  glGenFramebuffers(static_cast<GLsizei>(point_shadow_framebuffers_.size()), point_shadow_framebuffers_.data());
  for (int i = 0; i < scene_->point_lights().size(); ++i) {
    point_depth_maps_.emplace_back(kShadowMapSize, "pointDepthMaps", *texture_allocator_);
    glBindFramebuffer(GL_FRAMEBUFFER, point_shadow_framebuffers_[i]);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  for (int i = 0; i < scene.objects().size(); ++i) {
    LOG(INFO) << "Setting up object " << i << " of " << scene.objects().size() << " total objects";
    const GameObject &object = scene.objects()[i];
    RenderObject render_object;

    AttachMaterial(render_object, object.mesh->material);
    matrices_ubo_.Bind(shaders_[i].program(), "Matrices", kMatricesUBOBindingPoint);
    lights_.Bind(shaders_[i].program(), "Lights", kLightsUBOBindingPoint);
    light_space_matrices_.Bind(shaders_[i].program(), "LightSpaceMatrices", kLightSpaceMatricesUBOBindingPoint);

    render_object.object_index = i;
    render_object.shader_index = i;
    render_object.model = Uniform<glm::mat4>(shaders_[i].program(), "model", object.transform->GetMatrix());
    render_object.shadow_model =
        Uniform<glm::mat4>(point_shadow_shader_->program(), "model", object.transform->GetMatrix());
    render_object.normal_matrix = Uniform<glm::mat3>(
        shaders_[i].program(),
        "normalMatrix",
        glm::identity<glm::mat3>());

    glGenVertexArrays(1, &render_object.vao);
    glGenBuffers(1, &render_object.vbo);
    glGenBuffers(1, &render_object.ebo);

    glBindVertexArray(render_object.vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_object.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(object.mesh->vertices.size() * sizeof(Mesh::Vertex)),
                 object.mesh->vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_object.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(object.mesh->indices.size() * sizeof(GLuint)),
                 object.mesh->indices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Mesh::Vertex),
                          reinterpret_cast<void *>(offsetof(Mesh::Vertex, normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Mesh::Vertex),
                          reinterpret_cast<void *>(offsetof(Mesh::Vertex, texcoord)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Mesh::Vertex),
                          reinterpret_cast<void *>(offsetof(Mesh::Vertex, tangent)));

    glBindVertexArray(0);

    render_objects_.emplace_back(std::move(render_object));
  }
  LOG(INFO) << "Finished setup scene";
}

void Renderer::AttachMaterial(RenderObject &render_object, const Material &material) {
  std::vector<ShaderFlag> vertex_shader_flags{};
  std::vector<ShaderFlag> fragment_shader_flags{};

  if (material.ambient_texture.has_value()) {
    render_object.textures.emplace_back(material.ambient_texture.value(), "ambientTexture", *texture_allocator_);
  } else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoAmbientTexture, 1);
  }

  if (material.diffuse_texture.has_value()) {
    render_object.textures.emplace_back(material.diffuse_texture.value(), "diffuseTexture", *texture_allocator_);
  } else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoDiffuseTexture, 1);
  }

  if (material.specular_texture.has_value()) {
    render_object.textures.emplace_back(material.specular_texture.value(), "specularTexture", *texture_allocator_);
  } else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoSpecularTexture, 1);
  }

  if (material.shininess_texture.has_value()) {
    render_object.textures.emplace_back(material.shininess_texture.value(), "shininessTexture", *texture_allocator_);
  } else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoShininessTexture, 1);
  }

  if (material.alpha_texture.has_value()) {
    render_object.textures.emplace_back(material.alpha_texture.value(), "alphaTexture", *texture_allocator_);
  } else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoAlphaTexture, 1);
  }

  if (material.bump_texture.has_value()) {
    render_object.textures.emplace_back(material.bump_texture.value(), "bumpTexture", *texture_allocator_);
  } else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoBumpTexture, 1);
  }

  if (material.displacement_texture.has_value()) {
    render_object.textures.emplace_back(material.displacement_texture.value(),
                                        "displacementTexture",
                                        *texture_allocator_);
  } else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoDisplacementTexture, 1);
  }

  fragment_shader_flags.emplace_back(ShaderFlagTypes::kPointLightCount, scene_->point_lights().size());
  fragment_shader_flags.emplace_back(ShaderFlagTypes::kDirectionalLightCount, 1);
  fragment_shader_flags.emplace_back(ShaderFlagTypes::kSpotLightCount, scene_->spot_lights().size());

  shaders_.emplace_back("shaders/render_shader.vert",
                        vertex_shader_flags,
                        "shaders/render_shader.frag",
                        fragment_shader_flags,
                        *shader_allocator_);
  shaders_.back().Use();

  render_object.material_data = UniformBuffer(sizeof(MaterialUBOData));
  render_object.material_data.Bind(shaders_.back().program(), "Material", kMaterialUBOBindingPoint);
}
}
