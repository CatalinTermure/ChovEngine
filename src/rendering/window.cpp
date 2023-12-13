#include <SDL2/SDL_vulkan.h>
#include "rendering/window.h"

#include "absl/log/log.h"

namespace chove::rendering {

namespace {
SDL_Window *InitSDLWindow(SDL_WindowFlags flags) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    LOG(FATAL) << "Could not initialize SDL.";
    return nullptr;
  }

  if ((flags & SDL_WINDOW_OPENGL) == SDL_WINDOW_OPENGL) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  }

  SDL_Window *window = SDL_CreateWindow("Window",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        1280,
                                        720,
                                        flags);
  if (window == nullptr) {
    LOG(FATAL) << "Could not create SDL window.";
    return nullptr;
  }

  SDL_SetRelativeMouseMode(SDL_TRUE);

  return window;
}
}

Window::Window(SDL_WindowFlags flags) : window_(InitSDLWindow(flags), SDL_DestroyWindow) {}

int Window::height() {
  int height = 0;
  SDL_GetWindowSizeInPixels(window_.get(), nullptr, &height);
  return height;
}

int Window::width() {
  int width = 0;
  SDL_GetWindowSizeInPixels(window_.get(), &width, nullptr);
  return width;
}
}  // namespace chove::rendering
