#include <SDL2/SDL_vulkan.h>
#include "rendering/window.h"

#include "absl/log/log.h"

namespace chove::rendering {

namespace {
SDL_Window *InitSDLWindow() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    LOG(FATAL) << "Could not initialize SDL.";
    return nullptr;
  }

  SDL_Window *window =
      SDL_CreateWindow("Window",
                       SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED,
                       1280,
                       720,
                       SDL_WINDOW_OPENGL);
  if (window == nullptr) {
    LOG(FATAL) << "Could not create SDL window.";
    return nullptr;
  }

  SDL_SetRelativeMouseMode(SDL_TRUE);
  
  return window;
}
}

Window::Window() : window_(InitSDLWindow(), SDL_DestroyWindow) {}

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
