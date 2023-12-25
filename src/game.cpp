#include "game.h"

#include "rendering/opengl/renderer.h"

namespace chove {

namespace {
constexpr const float kCameraSpeed = 1e0F;
constexpr float kCameraRotationSpeed = 0.1F;
}

using rendering::Mesh;

Game::Game(RendererType renderer_type) : window_(static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    | SDL_WINDOW_ALLOW_HIGHDPI)) {
  renderer_ = std::make_unique<rendering::opengl::Renderer>(&window_);

  nanosuit = Mesh::ImportFromObj(std::filesystem::current_path() / "models" / "nanosuit" / "nanosuit.obj");
  sponza = Mesh::ImportFromObj(std::filesystem::current_path() / "models" / "sponza.obj");
  glm::vec3 nanosuit_position = {0.0F, 1.0F, 0.0F};
  glm::vec3 sponza_scale = glm::vec3(0.01F, 0.01F, 0.01F);

  objects::Scene scene;
  scene.camera() = objects::Camera(
      {0.0F, 0.0F, -1.0F, 1.0F},
      {0.0F, 0.0F, 1.0F},
      glm::radians(55.0F),
      static_cast<float>(window_.width()) / static_cast<float>(window_.height()),
      0.1F,
      100000.0F);

  for (auto &mesh : nanosuit) {
    scene.AddObject(mesh, {nanosuit_position,
                           glm::identity<glm::quat>(),
                           glm::vec3(1.0f, 1.0f, 1.0f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::identity<glm::quat>()});
  }

  for (auto &mesh : sponza) {
    scene.AddObject(mesh, {glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::identity<glm::quat>(),
                           sponza_scale,
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::identity<glm::quat>()});
  }
  scene.SetDirectionalLight({glm::vec3(0.0F, 1.0F, 1.0F),
                             glm::vec3(1.0F, 1.0F, 1.0F)});

//  scene.AddLight(objects::PointLight{1.0F,
//                                     0.00014F,
//                                     0.00000007F,
//                                     0.01F,
//                                     glm::vec3(0.5F, 2.0F, 0.0F),
//                                     100.0F,
//                                     glm::vec3(1.0F, 1.0F, 1.0F)
//  });

  scenes_["main"] = std::move(scene);

  camera_velocity_ = glm::vec3(0.0F, 0.0F, 0.0F);
  target_frame_rate_ = 60;
  SetCurrentScene("main");

  is_running_ = true;
}
void Game::HandleInput() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      is_running_ = false;
    } else if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:is_running_ = false;
          break;
        case SDLK_w:camera_velocity_.y = +kCameraSpeed;
          break;
        case SDLK_a:camera_velocity_.x = -kCameraSpeed;
          break;
        case SDLK_s:camera_velocity_.y = -kCameraSpeed;
          break;
        case SDLK_d:camera_velocity_.x = kCameraSpeed;
          break;
        case SDLK_SPACE:camera_velocity_.z = kCameraSpeed;
          break;
        case SDLK_LSHIFT:camera_velocity_.z = -kCameraSpeed;
          break;
        case SDLK_l:
          current_scene().SetDirectionalLight({current_scene().camera().look_direction(),
                                               glm::vec3(1.0F, 1.0F, 1.0F)});
          break;
        case SDLK_i:
          for (auto &mesh : nanosuit) {
            current_scene().AddObject(mesh, {current_scene().camera().position(),
                                             glm::identity<glm::quat>(),
                                             glm::vec3(1.0f, 1.0f, 1.0f),
                                             glm::vec3(0.0f, 0.0f, 0.0f),
                                             glm::identity<glm::quat>()});
          }
        default:break;
      }
    } else if (event.type == SDL_KEYUP) {
      switch (event.key.keysym.sym) {
        case SDLK_s:
        case SDLK_w:camera_velocity_.y = 0.0F;
          break;
        case SDLK_d:
        case SDLK_a:camera_velocity_.x = 0.0F;
          break;
        case SDLK_SPACE:
        case SDLK_LSHIFT:camera_velocity_.z = 0.0F;
          break;
        default: LOG(INFO) << std::format("Camera position is: ({},{},{})",
                                          current_scene().camera().position().x,
                                          current_scene().camera().position().y,
                                          current_scene().camera().position().z);
          LOG(INFO) << std::format("Camera look direction is: ({},{},{})",
                                   current_scene().camera().look_direction().x,
                                   current_scene().camera().look_direction().y,
                                   current_scene().camera().look_direction().z);

          break;
      }
    } else if (event.type == SDL_MOUSEMOTION) {
      current_scene().camera().Rotate(objects::Camera::RotationDirection::eRight,
                                      kCameraRotationSpeed * static_cast<float>(event.motion.xrel));
      current_scene().camera().Rotate(objects::Camera::RotationDirection::eUpward,
                                      kCameraRotationSpeed * static_cast<float>(event.motion.yrel));
    }
  }
}

void Game::HandlePhysics(std::chrono::duration<long long int, std::ratio<1, 1'000'000'000>> delta_time) {
  current_scene().camera().Move(objects::Camera::Direction::eForward,
                                camera_velocity_.y * static_cast<float>(std::chrono::nanoseconds(delta_time).count())
                                    / 4e7f);
  current_scene().camera().Move(objects::Camera::Direction::eRight,
                                camera_velocity_.x * static_cast<float>(std::chrono::nanoseconds(delta_time).count())
                                    / 4e7f);
  current_scene().camera().Move(objects::Camera::Direction::eUp,
                                camera_velocity_.z * static_cast<float>(std::chrono::nanoseconds(delta_time).count())
                                    / 4e7f);
}
} // namespace chove