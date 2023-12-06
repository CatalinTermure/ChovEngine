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

struct MaterialUBOData {
  float shininess;
  float opticalDensity;
  float dissolve;
  alignas(16) glm::vec3 diffuseColor;
  alignas(16) glm::vec3 ambientColor;
  alignas(16) glm::vec3 specularColor;
  alignas(16) glm::vec3 transmissionFilterColor;
};

constexpr int kMatricesUBOBindingPoint = 0;
constexpr int kMaterialUBOBindingPoint = 1;
constexpr int kLightsUBOBindingPoint = 2;

}

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

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glm::mat4 view_projection[2] = {scene_->camera().GetViewMatrix(), scene_->camera().GetProjectionMatrix()};
  view_projection_matrices_.UpdateData(view_projection, 2 * sizeof(glm::mat4));
  lights_.UpdateSubData(&scene_->directional_light(), 0, sizeof(objects::DirectionalLight));
  lights_.UpdateSubData(scene_->point_lights().data(),
                        sizeof(objects::DirectionalLight),
                        scene_->point_lights().size() * sizeof(objects::PointLight));
  lights_.UpdateSubData(scene_->spot_lights().data(),
                        sizeof(objects::DirectionalLight) + scene_->point_lights().size() * sizeof(objects::PointLight),
                        scene_->spot_lights().size() * sizeof(objects::SpotLight));

  MaterialUBOData
      material_ubo_data{};

  for (RenderObject &render_object : render_objects_) {
    shaders_[render_object.shader_index].Use();

    render_object.model.UpdateValue(scene_->objects()[render_object.object_index].transform->GetMatrix());
    render_object.normal_matrix.UpdateValue(
        glm::mat3(glm::inverseTranspose(
            view_projection[0] * scene_->objects()[render_object.object_index].transform->GetMatrix())));

    const Material &material = scene_->objects()[render_object.object_index].mesh->material;

    material_ubo_data.shininess = material.shininess;
    material_ubo_data.opticalDensity = material.optical_density;
    material_ubo_data.dissolve = material.dissolve;
    material_ubo_data.diffuseColor = material.diffuse_color;
    material_ubo_data.ambientColor = material.ambient_color;
    material_ubo_data.specularColor = material.specular_color;
    material_ubo_data.transmissionFilterColor = material.transmission_filter_color;

    render_object.material_data.UpdateData(&material_ubo_data, sizeof(MaterialUBOData));

    for (int i = 0; i < render_object.textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glUniform1i(glGetUniformLocation(shaders_[0].program(), render_object.textures[i].name().c_str()), i);
      glBindTexture(GL_TEXTURE_2D, render_object.textures[i].texture());
    }

    glBindVertexArray(render_object.vao);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(scene_->objects()[render_object.object_index].mesh->indices.size()),
                   GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);

    for (int i = 0; i < render_object.textures.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }

  SDL_GL_SwapWindow(*window_);
}

void Renderer::SetupScene(const objects::Scene &scene) {
  LOG(INFO) << "Starting setup scene";
  scene_ = &scene;
  LOG(INFO) << "Setting up shaders";

  LOG(INFO) << "Setting up uniforms";

  render_objects_.reserve(scene.objects().size());
  shaders_.reserve(scene.objects().size());

  view_projection_matrices_ = UniformBuffer(2 * sizeof(glm::mat4));
  lights_ = UniformBuffer(
      sizeof(objects::DirectionalLight) + scene_->point_lights().size() * sizeof(objects::PointLight)
          + scene_->spot_lights().size() * sizeof(objects::SpotLight));

  for (int i = 0; i < scene.objects().size(); ++i) {
    LOG(INFO) << "Setting up object " << i << " of " << scene.objects().size() << " total objects";
    const objects::GameObject &object = scene.objects()[i];
    RenderObject render_object;

    AttachMaterial(render_object, object.mesh->material);
    view_projection_matrices_.Bind(shaders_[i].program(), "Matrices", kMatricesUBOBindingPoint);
    lights_.Bind(shaders_[i].program(), "Lights", kLightsUBOBindingPoint);

    render_object.object_index = i;
    render_object.shader_index = i;
    render_object.model = Uniform<glm::mat4>(shaders_[i].program(), "model", object.transform->GetMatrix());
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
