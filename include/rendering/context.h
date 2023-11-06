#ifndef LABSEXTRA_INCLUDE_RENDERING_CONTEXT_H_
#define LABSEXTRA_INCLUDE_RENDERING_CONTEXT_H_

#include <memory>

#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <absl/status/statusor.h>

namespace chove::rendering {

struct QueueInfo {
  uint32_t family_index;
  std::vector<float> priorities;
  vk::raii::Queue queue;
};

class Context {
 public:
  [[nodiscard]] SDL_Window *window() { return window_.get(); }
  [[nodiscard]] vk::SurfaceKHR surface() { return *surface_; }
  [[nodiscard]] const vk::raii::Device &device() const { return device_; }
  [[nodiscard]] vk::PhysicalDevice physical_device() { return *physical_device_; }
  [[nodiscard]] QueueInfo &transfer_queue() { return transfer_queue_; }
  [[nodiscard]] QueueInfo &graphics_queue() { return graphics_queue_; }

  [[nodiscard]] static absl::StatusOr<Context> CreateContext();

 private:
  vk::raii::Context context_;
  vk::raii::Instance instance_;
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_;
  vk::raii::SurfaceKHR surface_;
  vk::raii::Device device_;
  vk::raii::PhysicalDevice physical_device_;
  QueueInfo transfer_queue_;
  QueueInfo graphics_queue_;

  Context(vk::raii::Context context,
          vk::raii::Instance instance,
          std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window,
          vk::raii::SurfaceKHR surface,
          vk::raii::Device device,
          vk::raii::PhysicalDevice physical_device,
          QueueInfo transfer_queue,
          QueueInfo graphics_queue)
      : context_(std::move(context)),
        instance_(std::move(instance)),
        window_(std::move(window)),
        surface_(std::move(surface)),
        device_(std::move(device)),
        physical_device_(std::move(physical_device)),
        transfer_queue_(std::move(transfer_queue)),
        graphics_queue_(std::move(graphics_queue)) {}
};
}
#endif //LABSEXTRA_INCLUDE_RENDERING_CONTEXT_H_
