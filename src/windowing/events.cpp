#include "windowing/events.h"

namespace chove::windowing {

WindowResizeEvent::WindowResizeEvent(WindowExtent new_extent) : new_extent_(new_extent) {}
EventType WindowResizeEvent::type() const { return EventType::kWindowResize; }
int WindowResizeEvent::new_width() const { return new_extent_.width; }
int WindowResizeEvent::new_height() const { return new_extent_.height; }
WindowExtent WindowResizeEvent::new_extent() const { return new_extent_; }

EventType WindowCloseEvent::type() const { return EventType::kWindowClose; }

KeyPressedEvent::KeyPressedEvent(KeyCode key_code) : key_code_(key_code) {}
EventType KeyPressedEvent::type() const { return EventType::kKeyPressed; }
KeyCode KeyPressedEvent::key_code() const { return key_code_; }

KeyReleasedEvent::KeyReleasedEvent(KeyCode key_code) : key_code_(key_code) {}
EventType KeyReleasedEvent::type() const { return EventType::kKeyReleased; }
KeyCode KeyReleasedEvent::key_code() const { return key_code_; }

MouseButtonPressedEvent::MouseButtonPressedEvent(MouseButton button) : button_(button) {}
EventType MouseButtonPressedEvent::type() const { return EventType::kMouseButtonPressed; }
MouseButton MouseButtonPressedEvent::button() const { return button_; }

MouseButtonReleasedEvent::MouseButtonReleasedEvent(MouseButton button) : button_(button) {}
EventType MouseButtonReleasedEvent::type() const { return EventType::kMouseButtonReleased; }
MouseButton MouseButtonReleasedEvent::button() const { return button_; }

} // namespace chove::windowing
