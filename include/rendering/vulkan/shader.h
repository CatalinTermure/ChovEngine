#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_SHADER_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_SHADER_H_

#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace chove::rendering::vulkan {
class Shader {
 public:
  Shader(const std::filesystem::path &path, vk::Device device);
  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&) noexcept;
  Shader &operator=(Shader &&) noexcept;

  [[nodiscard]] vk::ShaderModule module() const { return shader_; }
  [[nodiscard]] const std::vector<vk::DescriptorSetLayout> &descriptor_set_layouts() const { return descriptor_set_layouts_; }
  [[nodiscard]] const std::vector<vk::PushConstantRange> &push_constant_ranges() const { return push_constant_ranges_; }

  vk::DescriptorSetLayout AddDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding> &bindings);
  void AddPushConstantRanges(const std::vector<vk::PushConstantRange> &ranges);

  virtual ~Shader();
 private:
  vk::Device device_;
  vk::ShaderModule shader_;
  std::vector<vk::DescriptorSetLayout> descriptor_set_layouts_;
  std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_bindings_;
  std::vector<vk::PushConstantRange> push_constant_ranges_;
};
} // namespace chove::rendering::vulkan

#endif //CHOVENGINE_INCLUDE_RENDERING_VULKAN_SHADER_H_
