#include "rendering/opengl/renderer.h"

#include <GL/glew.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <format>
#include <glm/gtc/matrix_inverse.hpp>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/trigonometric.hpp"
#include "objects/game_object.h"
#include "objects/lights.h"
#include "objects/scene.h"
#include "rendering/material.h"
#include "rendering/mesh.h"
#include "rendering/opengl/render_object.h"
#include "rendering/opengl/shader.h"
#include "rendering/opengl/shader_allocator.h"
#include "rendering/opengl/shader_flags.h"
#include "rendering/opengl/texture.h"
#include "rendering/opengl/texture_allocator.h"
#include "rendering/opengl/uniform.h"
#include "windowing/window.h"

namespace chove::rendering::opengl {

using objects::DirectionalLight;
using objects::GameObject;
using objects::PointLight;
using objects::Scene;
using objects::SpotLight;
using objects::Transform;
using windowing::Window;

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
    case GL_DEBUG_SOURCE_API:
      msg += "Source: API";
      break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      msg += "Source: Window System";
      break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      msg += "Source: Shader Compiler";
      break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
      msg += "Source: Third Party";
      break;
    case GL_DEBUG_SOURCE_APPLICATION:
      msg += "Source: Application";
      break;
    case GL_DEBUG_SOURCE_OTHER:
      msg += "Source: Other";
      break;
    default:
      msg += "Source: Unknown";
      break;
  }
  msg += "\n";

  switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      msg += "Type: Error";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      msg += "Type: Deprecated Behaviour";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      msg += "Type: Undefined Behaviour";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      msg += "Type: Portability";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      msg += "Type: Performance";
      break;
    case GL_DEBUG_TYPE_MARKER:
      msg += "Type: Marker";
      break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
      msg += "Type: Push Group";
      break;
    case GL_DEBUG_TYPE_POP_GROUP:
      msg += "Type: Pop Group";
      break;
    case GL_DEBUG_TYPE_OTHER:
      msg += "Type: Other";
      break;
    default:
      msg += "Type: Unknown";
      break;
  }
  msg += "\n";

  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
      msg += "Severity: high";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      msg += "Severity: medium";
      break;
    case GL_DEBUG_SEVERITY_LOW:
      msg += "Severity: low";
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      msg += "Severity: notification";
      break;
    default:
      msg += "Severity: unknown";
      break;
  }
  msg += "\n-------------------------------------------------------------";
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
      LOG(ERROR) << msg;
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      LOG(WARNING) << msg;
      break;
    case GL_DEBUG_SEVERITY_LOW:
      LOG(INFO) << msg;
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      LOG(INFO) << msg;
      break;
    default:
      LOG(INFO) << msg;
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

constexpr std::array<glm::vec3, 6> kCubeMapDirections = {glm::vec3(1.0F, 0.0F, 0.0F),
                                                         glm::vec3(-1.0F, 0.0F, 0.0F),
                                                         glm::vec3(0.0F, 1.0F, 0.0F),
                                                         glm::vec3(0.0F, -1.0F, 0.0F),
                                                         glm::vec3(0.0F, 0.0F, 1.0F),
                                                         glm::vec3(0.0F, 0.0F, -1.0F)};

constexpr std::array<glm::vec3, 6> kCubeMapUpVectors = {glm::vec3(0.0F, -1.0F, 0.0F),
                                                        glm::vec3(0.0F, -1.0F, 0.0F),
                                                        glm::vec3(0.0F, 0.0F, 1.0F),
                                                        glm::vec3(0.0F, 0.0F, -1.0F),
                                                        glm::vec3(0.0F, -1.0F, 0.0F),
                                                        glm::vec3(0.0F, -1.0F, 0.0F)};

auto GetRenderInfo(Scene *scene) { return scene->GetAllObjectsWith<RenderObject, Transform, Mesh *>(); }

std::tuple<DirectionalLight, Texture &, GLuint> GetDirectionalLightInfo(Scene *scene) {
  auto view = scene->GetAllObjectsWith<DirectionalLight, Texture, GLuint>();
  DirectionalLight directional_light = view.get<DirectionalLight>(view.front());
  Texture &depth_map = view.get<Texture>(view.front());
  GLuint framebuffer = view.get<GLuint>(view.front());

  return {directional_light, depth_map, framebuffer};
}

auto GetPointLightsInfo(Scene *scene) { return scene->GetAllObjectsWith<PointLight, Texture, GLuint>(); }

auto GetSpotLightsInfo(Scene *scene) { return scene->GetAllObjectsWith<SpotLight, Texture, GLuint>(); }

} // namespace

Renderer::Renderer(const Window *window) : window_(window), scene_(nullptr) {
  glewExperimental = GL_TRUE;
  glewInit();

  glClearColor(0.3F, 0.3F, 0.3F, 1.0F);
  glViewport(0, 0, window_->extent().width, window_->extent().height);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_FRAMEBUFFER_SRGB);

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, nullptr);

  texture_allocator_ = std::make_unique<TextureAllocator>();
  shader_allocator_ = std::make_unique<ShaderAllocator>();

  depth_map_shader_ = std::make_unique<Shader>("shaders/depth_map.vert",
                                               std::vector<ShaderFlag>{},
                                               "shaders/depth_map.frag",
                                               std::vector<ShaderFlag>{},
                                               *shader_allocator_);

  white_pixel_ = std::make_unique<Texture>(
      std::filesystem::current_path() / "models" / "textures" / "white_pixel.png", "whitePixel", *texture_allocator_);
}

void Renderer::RenderDepthMap() {
  for (auto &&[_, render_info, transform, mesh] : GetRenderInfo(scene_).each()) {
    render_info.shadow_model.UpdateValue(transform.GetMatrix());
    glUniform1f(glGetUniformLocation(depth_map_shader_->program(), "dissolve"), mesh->material.dissolve);

    // find alphaTexture in textures
    auto alphaTexture = std::find_if(render_info.textures.begin(),
                                     render_info.textures.end(),
                                     [](const Texture &texture) { return texture.name() == "alphaTexture"; });
    if (alphaTexture == render_info.textures.end()) {
      glBindTexture(GL_TEXTURE_2D, white_pixel_->texture());
    }
    else {
      glBindTexture(GL_TEXTURE_2D, alphaTexture->texture());
    }

    glBindVertexArray(render_info.vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh->indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }
}

void Renderer::Render() {
  if (scene_ == nullptr) return;

  if (scene_->dirty_bit()) {
    SetupScene(*scene_);
  }

  // Sort objects by distance to camera
  GetRenderInfo(scene_).each([this](RenderObject &render_info, Transform &transform, Mesh *&mesh) {
    if (mesh->material.dissolve > 0.99F && !mesh->material.alpha_texture.has_value()) {
      render_info.dist = std::numeric_limits<float>::max();
      return;
    }

    render_info.dist = glm::distance2(scene_->camera().position(), transform.location + mesh->bounding_box.center());
  });
  scene_->registry().sort<RenderObject>(
      [](const RenderObject &lhs, const RenderObject &rhs) { return lhs.dist < rhs.dist; });

  // Start depth map render pass

  glViewport(0, 0, kShadowMapSize, kShadowMapSize);

  depth_map_shader_->Use();
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(glGetUniformLocation(depth_map_shader_->program(), "alphaTexture"), 0);
  for (auto &&[object, point_light, depth_map, framebuffer] : GetPointLightsInfo(scene_).each()) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    const glm::mat4 light_projection =
        glm::perspective(glm::radians(90.0F), 1.0F, point_light.near_plane, point_light.far_plane);
    for (int j = 0; j < 6; j++) {
      glFramebufferTexture2D(
          GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, depth_map.texture(), 0);
      glClear(GL_DEPTH_BUFFER_BIT);

      const glm::mat4 light_view =
          glm::lookAt(point_light.position, point_light.position + kCubeMapDirections.at(j), kCubeMapUpVectors.at(j));
      const glm::mat4 light_space_matrix = light_projection * light_view;
      Uniform<glm::mat4> light_space_matrix_uniform(
          depth_map_shader_->program(), "lightSpaceMatrix", light_space_matrix);
      light_space_matrix_uniform.UpdateValue(light_space_matrix);

      RenderDepthMap();
    }
  }

  depth_map_shader_->Use();
  {
    auto [directional_light, depth_map, framebuffer] = GetDirectionalLightInfo(scene_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    const glm::mat4 light_projection = glm::ortho(-20.0F, 20.0F, -20.0F, 20.0F, 0.1F, 100.0F);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map.texture(), 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    const glm::mat4 light_view =
        glm::lookAt(10.0F * glm::normalize(directional_light.direction), glm::vec3(0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
    glm::mat4 light_space_matrix = light_projection * light_view;
    Uniform<glm::mat4> light_space_matrix_uniform(depth_map_shader_->program(), "lightSpaceMatrix", light_space_matrix);
    light_space_matrix_uniform.UpdateValue(light_space_matrix);

    light_space_matrices_.UpdateSubData(&light_space_matrix, 0, sizeof(glm::mat4));

    RenderDepthMap();
  }

  depth_map_shader_->Use();
  size_t light_space_matrix_offset = sizeof(glm::mat4);
  for (auto &&[object, spot_light, depth_map, framebuffer] : GetSpotLightsInfo(scene_).each()) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    const glm::mat4 light_projection =
        glm::perspective(glm::radians(spot_light.outer_cutoff * 2.0F), 1.0F, 0.1F, 10.0F);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map.texture(), 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    const glm::mat4 light_view =
        glm::lookAt(spot_light.position, spot_light.position + spot_light.direction, glm::vec3(0.0F, 1.0F, 0.0F));
    glm::mat4 light_space_matrix = light_projection * light_view;
    Uniform<glm::mat4> light_space_matrix_uniform(depth_map_shader_->program(), "lightSpaceMatrix", light_space_matrix);
    light_space_matrix_uniform.UpdateValue(light_space_matrix);

    light_space_matrices_.UpdateSubData(&light_space_matrix, light_space_matrix_offset, sizeof(glm::mat4));
    light_space_matrix_offset += sizeof(glm::mat4);

    RenderDepthMap();
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Start actual rendering pass

  glViewport(0, 0, window_->extent().width, window_->extent().height);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  MatricesUBOData matrices_ubo_data = {scene_->camera().GetViewMatrix(), scene_->camera().GetProjectionMatrix()};
  matrices_ubo_.UpdateData(&matrices_ubo_data, sizeof(MatricesUBOData));
  matrices_ubo_.Rebind();

  // Send light data
  size_t lights_uniform_offset = 0;
  {
    auto view = scene_->GetAllObjectsWith<DirectionalLight>();
    DirectionalLight light = view.get<DirectionalLight>(view.front());
    light.direction = glm::vec3(scene_->camera().GetViewMatrix() * glm::vec4(light.direction, 0.0F));
    lights_.UpdateSubData(&light, 0, sizeof(DirectionalLight));
    lights_uniform_offset += sizeof(DirectionalLight);
  }

  {
    for (auto &&[_, point_light] : scene_->GetAllObjectsWith<PointLight>().each()) {
      PointLight light = point_light;
      light.positionEyeSpace = glm::vec3(scene_->camera().GetViewMatrix() * glm::vec4(light.position, 1.0F));
      lights_.UpdateSubData(&light, lights_uniform_offset, sizeof(PointLight));
      lights_uniform_offset += sizeof(PointLight);
    }
  }

  {
    for (auto &&[_, spot_light] : scene_->GetAllObjectsWith<SpotLight>().each()) {
      SpotLight light = spot_light;
      light.position = glm::vec3(scene_->camera().GetViewMatrix() * glm::vec4(light.position, 1.0F));
      light.direction = glm::vec3(scene_->camera().GetViewMatrix() * glm::vec4(light.direction, 0.0F));
      lights_.UpdateSubData(&light, lights_uniform_offset, sizeof(SpotLight));
      lights_uniform_offset += sizeof(SpotLight);
    }
  }
  lights_.Rebind();

  // Send object data

  MaterialUBOData material_ubo_data{};

  for (auto &&[_, render_info, transform, mesh] : GetRenderInfo(scene_).each()) {
    shaders_[render_info.shader_index].Use();

    light_space_matrices_.Rebind();

    glm::mat4 model_matrix = transform.GetMatrix();
    render_info.model.UpdateValue(model_matrix);
    render_info.normal_matrix.UpdateValue(glm::mat3(glm::inverseTranspose(matrices_ubo_data.view * model_matrix)));

    const Material &material = mesh->material;

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

    render_info.material_data.UpdateData(&material_ubo_data, sizeof(MaterialUBOData));
    render_info.material_data.Rebind();

    // Send shadow map data

    int texture_index = 0;
    int array_index = 0;
    for (auto &&[_, point_light, depth_map, framebuffer] : GetPointLightsInfo(scene_).each()) {
      glActiveTexture(GL_TEXTURE0 + texture_index);
      glUniform1i(glGetUniformLocation(shaders_[render_info.shader_index].program(),
                                       std::format("{}[{}]", depth_map.name(), array_index).c_str()),
                  texture_index);
      glBindTexture(GL_TEXTURE_CUBE_MAP, depth_map.texture());
      texture_index++;
      array_index++;
    }

    {
      auto [directional_light, depth_map, framebuffer] = GetDirectionalLightInfo(scene_);
      glActiveTexture(GL_TEXTURE0 + texture_index);
      glUniform1i(glGetUniformLocation(shaders_[render_info.shader_index].program(),
                                       std::format("{}[{}]", depth_map.name(), 0).c_str()),
                  texture_index);
      glBindTexture(GL_TEXTURE_2D, depth_map.texture());
      texture_index++;
    }

    array_index = 0;
    for (auto &&[_, spot_light, depth_map, framebuffer] : GetSpotLightsInfo(scene_).each()) {
      glActiveTexture(GL_TEXTURE0 + texture_index);
      glUniform1i(glGetUniformLocation(shaders_[render_info.shader_index].program(),
                                       std::format("{}[{}]", depth_map.name(), array_index).c_str()),
                  texture_index);
      glBindTexture(GL_TEXTURE_2D, depth_map.texture());
      texture_index++;
      array_index++;
    }

    for (int i = 0; i < render_info.textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + texture_index);
      const int location =
          glGetUniformLocation(shaders_[render_info.shader_index].program(), render_info.textures[i].name().c_str());
      glUniform1i(location, texture_index);
      glBindTexture(GL_TEXTURE_2D, render_info.textures[i].texture());
      texture_index++;
    }

    glBindVertexArray(render_info.vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh->indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    for (int i = 0; i < texture_index; ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }

  window_->SwapBuffers();
}

void Renderer::SetupScene(Scene &scene) {
  LOG(INFO) << "Starting setup scene";
  scene_ = &scene;
  scene_->ClearDirtyBit();

  scene_->RemoveComponentFromAll<RenderObject>();
  shaders_.clear();
  // Delete depth maps from lights
  scene_->RemoveComponentFromAll<Texture>();

  // Delete framebuffers from lights
  scene_->GetAllObjectsWith<GLuint>().each([](GLuint &framebuffer) { glDeleteBuffers(1, &framebuffer); });
  scene_->RemoveComponentFromAll<GLuint>();

  shaders_.reserve(scene_->GetAllObjectsWith<Mesh *>().size());

  matrices_ubo_ = UniformBuffer(2 * sizeof(glm::mat4));
  size_t point_light_count = scene_->GetAllObjectsWith<PointLight>().size();
  size_t spot_light_count = scene_->GetAllObjectsWith<SpotLight>().size();

  light_space_matrices_ = UniformBuffer((1 + spot_light_count) * sizeof(glm::mat4));
  lights_ = UniformBuffer(sizeof(DirectionalLight) + point_light_count * sizeof(PointLight) +
                          spot_light_count * sizeof(SpotLight));

  for (auto &&[entity, _] : scene_->GetAllObjectsWith<PointLight>().each()) {
    GLuint framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Texture depth_map(kShadowMapSize, "pointDepthMaps", *texture_allocator_);
    scene_->AddComponent<GLuint>(entity, std::move(framebuffer)); // TODO
    scene_->AddComponent<Texture>(entity, std::move(depth_map));
  }

  {
    auto view = scene_->GetAllObjectsWith<DirectionalLight>();
    auto entity = view.front();
    GLuint framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Texture depth_map(kShadowMapSize, kShadowMapSize, "directionalDepthMaps", *texture_allocator_);
    scene_->AddComponent<GLuint>(entity, std::move(framebuffer)); // TODO
    scene_->AddComponent<Texture>(entity, std::move(depth_map));
  }

  for (auto &&[entity, _] : scene_->GetAllObjectsWith<SpotLight>().each()) {
    GLuint framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Texture depth_map(kShadowMapSize, kShadowMapSize, "spotDepthMaps", *texture_allocator_);
    scene_->AddComponent<GLuint>(entity, std::move(framebuffer)); // TODO
    scene_->AddComponent<Texture>(entity, std::move(depth_map));
  }

  int index = 0;
  for (auto &&[entity, transform, mesh] : scene_->GetAllObjectsWith<Transform, Mesh *>().each()) {
    LOG(INFO) << "Setting up object " << index;
    RenderObject render_info;

    AttachMaterial(render_info, mesh->material);
    matrices_ubo_.Bind(shaders_[index].program(), "Matrices", kMatricesUBOBindingPoint);
    lights_.Bind(shaders_[index].program(), "Lights", kLightsUBOBindingPoint);
    light_space_matrices_.Bind(shaders_[index].program(), "LightSpaceMatrices", kLightSpaceMatricesUBOBindingPoint);

    if (mesh->material.dissolve > 0.99F && !mesh->material.alpha_texture.has_value()) {
      render_info.dist = std::numeric_limits<float>::max();
    }
    else {
      render_info.dist = glm::distance2(scene_->camera().position(), transform.location);
    }

    render_info.object_index = index;
    render_info.shader_index = index;
    render_info.model = Uniform<glm::mat4>(shaders_[index].program(), "model", transform.GetMatrix());
    render_info.shadow_model = Uniform<glm::mat4>(depth_map_shader_->program(), "model", transform.GetMatrix());
    render_info.normal_matrix =
        Uniform<glm::mat3>(shaders_[index].program(), "normalMatrix", glm::identity<glm::mat3>());

    glGenVertexArrays(1, &render_info.vao);
    glGenBuffers(1, &render_info.vbo);
    glGenBuffers(1, &render_info.ebo);

    glBindVertexArray(render_info.vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_info.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh->vertices.size() * sizeof(Mesh::Vertex)),
                 mesh->vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_info.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh->indices.size() * sizeof(GLuint)),
                 mesh->indices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), reinterpret_cast<void *>(offsetof(Mesh::Vertex, normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), reinterpret_cast<void *>(offsetof(Mesh::Vertex, texcoord)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), reinterpret_cast<void *>(offsetof(Mesh::Vertex, tangent)));

    glBindVertexArray(0);

    scene_->AddComponent<RenderObject>(entity, std::move(render_info));
    index++;
  }

  LOG(INFO) << "Finished setup scene";
}

void Renderer::AttachMaterial(RenderObject &render_object, const Material &material) {
  std::vector<ShaderFlag> vertex_shader_flags{};
  std::vector<ShaderFlag> fragment_shader_flags{};

  const std::vector<ShaderFlag> shadow_vertex_shader_flags{};
  const std::vector<ShaderFlag> shadow_fragment_shader_flags{};

  if (material.ambient_texture.has_value()) {
    render_object.textures.emplace_back(material.ambient_texture.value(), "ambientTexture", *texture_allocator_);
  }
  else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoAmbientTexture, 1);
  }

  if (material.diffuse_texture.has_value()) {
    render_object.textures.emplace_back(material.diffuse_texture.value(), "diffuseTexture", *texture_allocator_);
  }
  else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoDiffuseTexture, 1);
  }

  if (material.specular_texture.has_value()) {
    render_object.textures.emplace_back(material.specular_texture.value(), "specularTexture", *texture_allocator_);
  }
  else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoSpecularTexture, 1);
  }

  if (material.shininess_texture.has_value()) {
    render_object.textures.emplace_back(material.shininess_texture.value(), "shininessTexture", *texture_allocator_);
  }
  else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoShininessTexture, 1);
  }

  if (material.alpha_texture.has_value()) {
    render_object.textures.emplace_back(material.alpha_texture.value(), "alphaTexture", *texture_allocator_);
  }
  else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoAlphaTexture, 1);
  }

  if (material.bump_texture.has_value()) {
    render_object.textures.emplace_back(material.bump_texture.value(), "bumpTexture", *texture_allocator_);
  }
  else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoBumpTexture, 1);
  }

  if (material.displacement_texture.has_value()) {
    render_object.textures.emplace_back(
        material.displacement_texture.value(), "displacementTexture", *texture_allocator_);
  }
  else {
    fragment_shader_flags.emplace_back(ShaderFlagTypes::kNoDisplacementTexture, 1);
  }

  fragment_shader_flags.emplace_back(ShaderFlagTypes::kPointLightCount,
                                     static_cast<int>(scene_->GetAllObjectsWith<PointLight>().size()));
  fragment_shader_flags.emplace_back(ShaderFlagTypes::kDirectionalLightCount, 1);
  fragment_shader_flags.emplace_back(ShaderFlagTypes::kSpotLightCount,
                                     static_cast<int>(scene_->GetAllObjectsWith<SpotLight>().size()));

  vertex_shader_flags.emplace_back(ShaderFlagTypes::kPointLightCount,
                                   static_cast<int>(scene_->GetAllObjectsWith<PointLight>().size()));
  vertex_shader_flags.emplace_back(ShaderFlagTypes::kDirectionalLightCount, 1);
  vertex_shader_flags.emplace_back(ShaderFlagTypes::kSpotLightCount,
                                   static_cast<int>(scene_->GetAllObjectsWith<SpotLight>().size()));

  shaders_.emplace_back("shaders/render_shader.vert",
                        vertex_shader_flags,
                        "shaders/render_shader.frag",
                        fragment_shader_flags,
                        *shader_allocator_);
  shaders_.back().Use();

  render_object.material_data = UniformBuffer(sizeof(MaterialUBOData));
  render_object.material_data.Bind(shaders_.back().program(), "Material", kMaterialUBOBindingPoint);
}
} // namespace chove::rendering::opengl
