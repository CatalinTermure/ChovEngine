#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_SHADER_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_SHADER_H_

#include <memory>
#include <vulkan/vulkan_raii.hpp>
#include <filesystem>
#include <vector>
#include <span>

#include "context.h"

namespace chove::rendering::vulkan {
class Shader {
 public:
  Shader(const std::filesystem::path &path, const Context &context);
  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&);
  Shader &operator=(Shader &&);

  [[nodiscard]] vk::raii::ShaderModule &module() { return shader_; }
  [[nodiscard]] std::span<vk::raii::DescriptorSetLayout> descriptor_set_layouts() { return descriptor_set_layouts_; }
  [[nodiscard]] std::span<vk::PushConstantRange> push_constant_ranges() { return push_constant_ranges_; }

  vk::DescriptorSetLayout AddDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding> &bindings);
  void AddPushConstantRanges(const std::vector<vk::PushConstantRange> &ranges);

  virtual ~Shader() = default;
 private:
  const Context *context_;
  vk::raii::ShaderModule shader_;
  std::vector<vk::raii::DescriptorSetLayout> descriptor_set_layouts_;
  std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_bindings_;
  std::vector<vk::PushConstantRange> push_constant_ranges_;
};
} // namespace chove::rendering

#endif //CHOVENGINE_INCLUDE_RENDERING_VULKAN_SHADER_H_
