#include "game.h"

#include "rendering/opengl/renderer.h"

namespace chove {

Game::Game(RendererType renderer_type) : window_(static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    | SDL_WINDOW_ALLOW_HIGHDPI)) {
  renderer_ = std::make_unique<rendering::opengl::Renderer>(&window_);
  target_frame_rate_ = 60;
}
} // namespace chove