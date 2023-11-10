#ifndef LABSEXTRA_INCLUDE_RENDERING_WINDOW_H_
#define LABSEXTRA_INCLUDE_RENDERING_WINDOW_H_

#include <SDL2/SDL.h>
#include <memory>

namespace chove::rendering {

class Window {
 public:
  Window();

  [[nodiscard]] int width();
  [[nodiscard]] int height();

  operator SDL_Window *() { return window_.get(); }
 private:
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_;
};

}  // namespace chove::rendering

#endif //LABSEXTRA_INCLUDE_RENDERING_WINDOW_H_
