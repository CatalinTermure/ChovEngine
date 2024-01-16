#ifndef CHOVENGINE_INCLUDE_RENDERING_WINDOW_H_
#define CHOVENGINE_INCLUDE_RENDERING_WINDOW_H_

#include <SDL2/SDL.h>
#include <memory>

namespace chove::rendering {

class Window {
 public:
  explicit Window(SDL_WindowFlags flags);

  [[nodiscard]] int width();
  [[nodiscard]] int height();

  operator SDL_Window *() { return window_.get(); }
 private:
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_;
};

}  // namespace chove::rendering

#endif //CHOVENGINE_INCLUDE_RENDERING_WINDOW_H_
