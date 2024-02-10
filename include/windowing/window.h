#ifndef FLUIDSIM_INCLUDE_WINDOWING_WINDOW_H_
#define FLUIDSIM_INCLUDE_WINDOWING_WINDOW_H_

#include "windowing/events.h"

#include <GL/glew.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <deque>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace chove::windowing {

enum class RendererType : uint8_t {
  kOpenGL,
  kVulkan
};

class Window {
 public:
  static Window Create(const std::string &title, WindowExtent extent, RendererType renderer_type);

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
  Window(Window &&) = delete;
  Window &operator=(Window &&) = delete;
  ~Window();

  [[nodiscard]] Event *GetEvent();
  [[nodiscard]] WindowExtent extent() const;
  [[nodiscard]] WindowPosition mouse_position() const;

  void PushEvent(std::unique_ptr<Event> event);
  void HandleEvent(Event *event);
  void ReturnEvent(Event *event);

  void PollEvents();

  void SwapBuffers() const;

  [[nodiscard]] VkSurfaceKHR CreateSurface(VkInstance instance) const;
  [[nodiscard]] static std::vector<const char *> GetRequiredVulkanExtensions();
 private:
  std::deque<std::unique_ptr<Event>> event_queue_;
  std::mutex event_queue_mutex_{};
  WindowPosition mouse_position_{};
  GLFWwindow *window_ptr_;

  explicit Window(GLFWwindow *window_ptr);
};
} // namespace chove::windowing

#endif //FLUIDSIM_INCLUDE_WINDOWING_WINDOW_H_
