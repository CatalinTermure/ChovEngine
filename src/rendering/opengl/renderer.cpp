#include "rendering/opengl/renderer.h"

namespace chove::rendering::opengl {

#include <GL/glew.h>

Renderer::Renderer(Window *window) : window_(window) {
  glewExperimental = GL_TRUE;
  glewInit();
}

void Renderer::Render() {

}

void Renderer::SetupScene(const objects::Scene &scene) {

}
}
