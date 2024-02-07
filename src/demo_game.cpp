#include "demo_game.h"

#include <absl/log/log.h>

namespace chove {

namespace {
constexpr const float kCameraSpeed = 1e0F;
constexpr const float kCameraVelocityConstant = 1e7F;
constexpr float kCameraRotationSpeed = 0.1F;

} // namespace

using rendering::Mesh;
using objects::Scene;
using objects::Camera;
using objects::Transform;
using objects::DirectionalLight;
using objects::PointLight;
using objects::SpotLight;
using windowing::Event;
using windowing::EventType;
using windowing::KeyPressedEvent;
using windowing::KeyCode;
using windowing::WindowPosition;

void DemoGame::Initialize() {
  objects::Scene scene;
  glm::vec3 nanosuit_position = {0.0F, 1.0F, 0.0F};
  glm::vec3 cube_position = {2.0F, 1.0F, 0.0F};
  glm::vec3 sponza_scale = glm::vec3(0.01F, 0.01F, 0.01F);

  nanosuit = object_manager_.ImportObject(std::filesystem::current_path() / "models" / "bricks" / "plane.obj",
                                          Transform{nanosuit_position,
                                                    glm::identity<glm::quat>(),
                                                    glm::vec3(1.0F),
                                                    nullptr},
                                          scene);
  cube = object_manager_.ImportObject(std::filesystem::current_path() / "models" / "bricks" / "plane2.obj",
                                      Transform{cube_position,
                                                glm::identity<glm::quat>(),
                                                glm::vec3(1.0F),
                                                nullptr},
                                      scene);
//  sponza = object_manager_.ImportObject(std::filesystem::current_path() / "models" / "sponza.obj",
//                                        Transform{glm::vec3(0.0F),
//                                                  glm::identity<glm::quat>(),
//                                                  sponza_scale,
//                                                  nullptr},
//                                        scene);

  scene.camera() = objects::Camera(
      glm::vec4{0.0F, 0.0F, -1.0F, 1.0F},
      glm::vec3{0.0F, 0.0F, 1.0F},
      glm::radians(55.0F),
      static_cast<float>(window_.extent().width) / static_cast<float>(window_.extent().height),
      0.1F,
      10000.0F);

  scene.SetDirectionalLight({glm::vec3(0.01F, 1.0F, 0.01F),
                             0.2F,
                             glm::vec3(1.0F, 1.0F, 1.0F)});

  scene.AddLight(objects::PointLight{1.0F,
                                     0.0014F,
                                     0.00007F,
                                     0.01F,
                                     glm::vec3(2.5F, 6.0F, 0.0F),
                                     100.0F,
                                     glm::vec3(1.0F, 1.0F, 1.0F),
                                     0.2F,
  });

  scenes_["main"] = std::move(scene);

  SetCurrentScene("main");

  is_running_ = true;
}

void DemoGame::HandleInput() {
  window_.PollEvents();

  WindowPosition current_mouse_position = window_.mouse_position();
  WindowPosition mouse_delta = {current_mouse_position.x - last_mouse_position_.x,
                                current_mouse_position.y - last_mouse_position_.y};
  current_scene().camera().Rotate(objects::Camera::RotationDirection::eRight,
                                  kCameraRotationSpeed * static_cast<float>(mouse_delta.x));
  current_scene().camera().Rotate(objects::Camera::RotationDirection::eUpward,
                                  kCameraRotationSpeed * static_cast<float>(mouse_delta.y));
  last_mouse_position_ = current_mouse_position;

  Event *event = window_.GetEvent();
  while (event != nullptr) {
    if (event->type() == EventType::kWindowClose) {
      is_running_ = false;
    } else if (event->type() == EventType::kKeyPressed) {
      auto *key_event = dynamic_cast<KeyPressedEvent *>(event);
      switch (key_event->key_code()) {
        case KeyCode::kEscape:
          is_running_ = false;
          break;
        case KeyCode::kW:
          camera_velocity_.y = +kCameraSpeed;
          break;
        case KeyCode::kA:
          camera_velocity_.x = -kCameraSpeed;
          break;
        case KeyCode::kS:
          camera_velocity_.y = -kCameraSpeed;
          break;
        case KeyCode::kD:
          camera_velocity_.x = kCameraSpeed;
          break;
        case KeyCode::kSpace:
          camera_velocity_.z = kCameraSpeed;
          break;
        case KeyCode::kLeftShift:
          camera_velocity_.z = -kCameraSpeed;
          break;
        case KeyCode::kL:
          current_scene().SetDirectionalLight({-current_scene().camera().look_direction(),
                                               0.2F,
                                               glm::vec3(1.0F, 1.0F, 1.0F)});
          break;
        case KeyCode::kI:
          object_manager_.ImportObject(std::filesystem::current_path() / "models" / "bricks" / "plane.obj",
                                       objects::Transform{current_scene().camera().position(),
                                                          glm::identity<glm::quat>(),
                                                          glm::vec3(1.0f, 1.0f, 1.0f),
                                                          nullptr}, current_scene());
        default:
          break;
      }
    } else if (event->type() == EventType::kKeyReleased) {
      auto *key_event = dynamic_cast<windowing::KeyReleasedEvent *>(event);
      switch (key_event->key_code()) {
        case KeyCode::kS:
        case KeyCode::kW:
          camera_velocity_.y = 0.0F;
          break;
        case KeyCode::kD:
        case KeyCode::kA:
          camera_velocity_.x = 0.0F;
          break;
        case KeyCode::kSpace:
        case KeyCode::kLeftShift:
          camera_velocity_.z = 0.0F;
          break;
        default:
          LOG(INFO) << std::format("Camera position is: ({},{},{})",
                                   current_scene().camera().position().x,
                                   current_scene().camera().position().y,
                                   current_scene().camera().position().z);
          LOG(INFO) << std::format("Camera look direction is: ({},{},{})",
                                   current_scene().camera().look_direction().x,
                                   current_scene().camera().look_direction().y,
                                   current_scene().camera().look_direction().z);

          break;
      }
    }
    window_.HandleEvent(event);
    event = window_.GetEvent();
  }
}
void DemoGame::HandlePhysics(std::chrono::duration<long long int, std::ratio<1, 1'000'000'000>> delta_time) {
  current_scene().camera().Move(objects::Camera::Direction::eForward,
                                camera_velocity_.y * static_cast<float>(std::chrono::nanoseconds(delta_time).count())
                                    / kCameraVelocityConstant);
  current_scene().camera().Move(objects::Camera::Direction::eRight,
                                camera_velocity_.x * static_cast<float>(std::chrono::nanoseconds(delta_time).count())
                                    / kCameraVelocityConstant);
  current_scene().camera().Move(objects::Camera::Direction::eUp,
                                camera_velocity_.z * static_cast<float>(std::chrono::nanoseconds(delta_time).count())
                                    / kCameraVelocityConstant);
}

DemoGame::DemoGame(windowing::RendererType renderer_type) : Application(renderer_type) {}
}  // namespace chove