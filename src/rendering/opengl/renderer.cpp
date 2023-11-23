#include "rendering/opengl/renderer.h"

#include <GL/glew.h>

namespace chove::rendering::opengl {

Renderer::Renderer(Window *window) : window_(window), scene_(nullptr), view_(), projection_() {
  context_ = SDL_GL_CreateContext(*window_);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

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
}

void Renderer::Render() {
  if (scene_ == nullptr) return;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  shaders_[0].Use();

  view_.UpdateValue(scene_->camera().GetViewMatrix());
  projection_.UpdateValue(scene_->camera().GetProjectionMatrix());

  for (auto &render_object : render_objects_) {
    render_object.model.UpdateValue(scene_->objects()[render_object.object_index].transform->GetMatrix());

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
  scene_ = &scene;
  shaders_.emplace_back("shaders/render_shader.vert", "shaders/render_shader.frag");

  view_ = Uniform<glm::mat4>(shaders_[0].program(), "view", scene_->camera().GetViewMatrix());
  projection_ = Uniform<glm::mat4>(shaders_[0].program(), "projection", scene_->camera().GetProjectionMatrix());

  render_objects_.reserve(scene.objects().size());
  for (int i = 0; i < scene.objects().size(); ++i) {
    const objects::GameObject &object = scene.objects()[i];
    RenderObject render_object;

    render_object.object_index = i;
    render_object.model = Uniform<glm::mat4>(shaders_[0].program(), "model", object.transform->GetMatrix());

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

    AttachMaterial(render_object, object.mesh->material);

    render_objects_.emplace_back(std::move(render_object));
  }
}

void Renderer::AttachMaterial(RenderObject &render_object, const Material &material) {
  if (material.diffuse_texture.has_value()) {
    render_object.textures.emplace_back(material.diffuse_texture.value(), "diffuseTexture");
  }
}
}
