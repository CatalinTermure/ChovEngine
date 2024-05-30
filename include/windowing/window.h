#ifndef CHOVENGINE_INCLUDE_WINDOWING_WINDOW_H_
#define CHOVENGINE_INCLUDE_WINDOWING_WINDOW_H_

#include "windowing/events.h"

#include <readerwriterqueue.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <GL/glew.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <deque>
#include <memory>
#include <string_view>
#include <vector>

namespace chove::windowing {

enum class RendererType : uint8_t { kOpenGL, kVulkan };

class Window {
 public:
  static Window Create(const std::string &title, WindowExtent extent, RendererType renderer_type);

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
  Window(Window &&) = delete;
  Window &operator=(Window &&) = delete;
  ~Window();

  [[nodiscard]] std::unique_ptr<Event> GetEvent();
  [[nodiscard]] std::unique_ptr<Event> GetRendererEvent();
  [[nodiscard]] Event *PeekRendererEvent() const;
  [[nodiscard]] WindowExtent extent() const;
  [[nodiscard]] WindowPosition mouse_position() const;

  void SetLockedCursor(bool locked) const;
  void PollEvents();

  void SwapBuffers() const;

  [[nodiscard]] VkSurfaceKHR CreateSurface(VkInstance instance) const;
  [[nodiscard]] static std::vector<const char *> GetRequiredVulkanExtensions();

 private:
  moodycamel::ReaderWriterQueue<std::unique_ptr<Event>> renderer_event_queue_;
  std::deque<std::unique_ptr<Event>> application_event_queue_;
  WindowPosition mouse_position_{};
  GLFWwindow *window_ptr_;

  explicit Window(GLFWwindow *window_ptr);
};
}  // namespace chove::windowing

#endif  // CHOVENGINE_INCLUDE_WINDOWING_WINDOW_H_
