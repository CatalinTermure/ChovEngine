#include "demo_game.h"

int main() {
  chove::DemoGame game{chove::windowing::RendererType::kOpenGL};
  game.Run();
  return 0;
}
