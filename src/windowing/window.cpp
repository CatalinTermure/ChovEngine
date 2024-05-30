#include "windowing/window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <span>
#include <stdexcept>

namespace chove::windowing {

namespace {

KeyCode GLFWKeyCodeToChovEKeyCode(int key) {
  switch (key) {
    case GLFW_KEY_A:
      return KeyCode::kA;
    case GLFW_KEY_B:
      return KeyCode::kB;
    case GLFW_KEY_C:
      return KeyCode::kC;
    case GLFW_KEY_D:
      return KeyCode::kD;
    case GLFW_KEY_E:
      return KeyCode::kE;
    case GLFW_KEY_F:
      return KeyCode::kF;
    case GLFW_KEY_G:
      return KeyCode::kG;
    case GLFW_KEY_H:
      return KeyCode::kH;
    case GLFW_KEY_I:
      return KeyCode::kI;
    case GLFW_KEY_J:
      return KeyCode::kJ;
    case GLFW_KEY_K:
      return KeyCode::kK;
    case GLFW_KEY_L:
      return KeyCode::kL;
    case GLFW_KEY_M:
      return KeyCode::kM;
    case GLFW_KEY_N:
      return KeyCode::kN;
    case GLFW_KEY_O:
      return KeyCode::kO;
    case GLFW_KEY_P:
      return KeyCode::kP;
    case GLFW_KEY_Q:
      return KeyCode::kQ;
    case GLFW_KEY_R:
      return KeyCode::kR;
    case GLFW_KEY_S:
      return KeyCode::kS;
    case GLFW_KEY_T:
      return KeyCode::kT;
    case GLFW_KEY_U:
      return KeyCode::kU;
    case GLFW_KEY_V:
      return KeyCode::kV;
    case GLFW_KEY_W:
      return KeyCode::kW;
    case GLFW_KEY_X:
      return KeyCode::kX;
    case GLFW_KEY_Y:
      return KeyCode::kY;
    case GLFW_KEY_Z:
      return KeyCode::kZ;
    case GLFW_KEY_0:
      return KeyCode::k0;
    case GLFW_KEY_1:
      return KeyCode::k1;
    case GLFW_KEY_2:
      return KeyCode::k2;
    case GLFW_KEY_3:
      return KeyCode::k3;
    case GLFW_KEY_4:
      return KeyCode::k4;
    case GLFW_KEY_5:
      return KeyCode::k5;
    case GLFW_KEY_6:
      return KeyCode::k6;
    case GLFW_KEY_7:
      return KeyCode::k7;
    case GLFW_KEY_8:
      return KeyCode::k8;
    case GLFW_KEY_9:
      return KeyCode::k9;
    case GLFW_KEY_SPACE:
      return KeyCode::kSpace;
    case GLFW_KEY_APOSTROPHE:
      return KeyCode::kApostrophe;
    case GLFW_KEY_COMMA:
      return KeyCode::kComma;
    case GLFW_KEY_MINUS:
      return KeyCode::kMinus;
    case GLFW_KEY_PERIOD:
      return KeyCode::kPeriod;
    case GLFW_KEY_SLASH:
      return KeyCode::kSlash;
    case GLFW_KEY_SEMICOLON:
      return KeyCode::kSemicolon;
    case GLFW_KEY_EQUAL:
      return KeyCode::kEqual;
    case GLFW_KEY_LEFT_SHIFT:
      return KeyCode::kLeftShift;
    case GLFW_KEY_LEFT_CONTROL:
      return KeyCode::kLeftControl;
    case GLFW_KEY_LEFT_ALT:
      return KeyCode::kLeftAlt;
    case GLFW_KEY_RIGHT_SHIFT:
      return KeyCode::kRightShift;
    case GLFW_KEY_RIGHT_CONTROL:
      return KeyCode::kRightControl;
    case GLFW_KEY_RIGHT_ALT:
      return KeyCode::kRightAlt;
    case GLFW_KEY_ESCAPE:
      return KeyCode::kEscape;
    default:
      return KeyCode::kUnknown;
  }
}

MouseButton GLFWMouseButtonToChovEMouseButton(int button) {
  switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      return MouseButton::kLeft;
    case GLFW_MOUSE_BUTTON_RIGHT:
      return MouseButton::kRight;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      return MouseButton::kMiddle;
    case GLFW_MOUSE_BUTTON_4:
      return MouseButton::kButton4;
    case GLFW_MOUSE_BUTTON_5:
      return MouseButton::kButton5;
    case GLFW_MOUSE_BUTTON_6:
      return MouseButton::kButton6;
    case GLFW_MOUSE_BUTTON_7:
      return MouseButton::kButton7;
    case GLFW_MOUSE_BUTTON_8:
      return MouseButton::kButton8;
    default:
      return MouseButton::kLeft;
  }
}

}  // namespace

Window Window::Create(const std::string &title, WindowExtent extent, RendererType renderer_type) {
  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  if (renderer_type == RendererType::kVulkan) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  }
  else if (renderer_type == RendererType::kOpenGL) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  }
  else {
    throw std::runtime_error("Invalid renderer type");
  }

  GLFWwindow *window_ptr =
      glfwCreateWindow(static_cast<int>(extent.width), static_cast<int>(extent.height), title.data(), nullptr, nullptr);
  if (window_ptr == nullptr) {
    throw std::runtime_error("Failed to create window");
  }

  if (renderer_type == RendererType::kOpenGL) {
    glfwMakeContextCurrent(window_ptr);
    glfwSwapInterval(1);
  }

  glfwSetKeyCallback(window_ptr, [](GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
    auto *chove_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
    switch (action) {
      case GLFW_PRESS:
        chove_window->application_event_queue_.push_back(std::make_unique<KeyPressedEvent>(GLFWKeyCodeToChovEKeyCode(key
        )));
        break;
      case GLFW_RELEASE:
        chove_window->application_event_queue_.push_back(
            std::make_unique<KeyReleasedEvent>(GLFWKeyCodeToChovEKeyCode(key))
        );
        break;
      default:
        break;
    }
  });

  glfwSetMouseButtonCallback(window_ptr, [](GLFWwindow *window, int button, int action, int /*mods*/) {
    auto *chove_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
    switch (action) {
      case GLFW_PRESS:
        chove_window->application_event_queue_.push_back(
            std::make_unique<MouseButtonPressedEvent>(GLFWMouseButtonToChovEMouseButton(button))
        );
        break;
      case GLFW_RELEASE:
        chove_window->application_event_queue_.push_back(
            std::make_unique<MouseButtonReleasedEvent>(GLFWMouseButtonToChovEMouseButton(button))
        );
        break;
      default:
        break;
    }
  });

  glfwSetWindowSizeCallback(window_ptr, [](GLFWwindow *window, int width, int height) {
    auto *chove_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
    chove_window->renderer_event_queue_.enqueue(
        std::make_unique<WindowResizeEvent>(WindowExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)})
    );
  });

  glfwSetCursorPosCallback(window_ptr, [](GLFWwindow *window, double x_pos, double y_pos) {
    auto *chove_window = static_cast<Window *>(glfwGetWindowUserPointer(window));
    chove_window->mouse_position_ = {static_cast<int>(x_pos), static_cast<int>(y_pos)};
  });

  return Window{window_ptr};
}

WindowExtent Window::extent() const {
  int width = 0;
  int height = 0;
  glfwGetWindowSize(window_ptr_, &width, &height);
  return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

std::unique_ptr<Event> Window::GetEvent() {
  if (application_event_queue_.empty()) return nullptr;
  std::unique_ptr<Event> result = std::move(application_event_queue_.front());
  application_event_queue_.pop_front();
  return result;
}

std::unique_ptr<Event> Window::GetRendererEvent() {
  std::unique_ptr<Event> event;
  const bool dequeued = renderer_event_queue_.try_dequeue(event);
  if (!dequeued) return nullptr;

  return event;
}

Event *Window::PeekRendererEvent() const { return renderer_event_queue_.peek()->get(); }

void Window::PollEvents() {
  glfwPollEvents();

  if (glfwWindowShouldClose(window_ptr_) != 0) {
    application_event_queue_.push_back(std::make_unique<WindowCloseEvent>());
  }
}

WindowPosition Window::mouse_position() const { return mouse_position_; }

void Window::SetLockedCursor(bool locked) const {
  if (locked) {
    glfwSetInputMode(window_ptr_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }
  else {
    auto [width, height] = extent();
    glfwSetInputMode(window_ptr_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPos(window_ptr_, width / 2.0, height / 2.0);
  }
}

VkSurfaceKHR Window::CreateSurface(VkInstance instance) const {
  if (glfwVulkanSupported() != GLFW_TRUE) {
    throw std::runtime_error("Vulkan not supported");
  }

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  if (glfwCreateWindowSurface(instance, window_ptr_, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface");
  }
  return surface;
}

std::vector<const char *> Window::GetRequiredVulkanExtensions() {
  uint32_t glfw_extension_count = 0;
  const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  std::span extensions(glfw_extensions, glfw_extension_count);
  std::vector result(extensions.begin(), extensions.end());
  return result;
}

Window::Window(GLFWwindow *window_ptr) : window_ptr_(window_ptr) {
  glfwSetWindowUserPointer(window_ptr, this);

  glfwSetInputMode(window_ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  {
    double x_pos = NAN;
    double y_pos = NAN;
    glfwGetCursorPos(window_ptr, &x_pos, &y_pos);
    mouse_position_ = {static_cast<int>(x_pos), static_cast<int>(y_pos)};
  }
}

Window::~Window() {
  glfwDestroyWindow(window_ptr_);
  glfwTerminate();
}

void Window::SwapBuffers() const { glfwSwapBuffers(window_ptr_); }

}  // namespace chove::windowing
