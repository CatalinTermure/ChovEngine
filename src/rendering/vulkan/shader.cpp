#include "rendering/vulkan/shader.h"

#include <fstream>

namespace chove::rendering::vulkan {

namespace {

vk::ShaderModule CreateShaderModule(const std::filesystem::path &shader_file_path, const vk::Device &device) {
  std::ifstream shader_file{shader_file_path, std::ios::binary};
  shader_file.seekg(0, std::ios::end);
  auto shader_file_size = shader_file.tellg();
  shader_file.seekg(0, std::ios::beg);
  std::vector<char> buffer(shader_file_size);
  shader_file.read(buffer.data(), shader_file_size);
  std::span<uint32_t> shader_code{reinterpret_cast<uint32_t *>(buffer.data()), // NOLINT(*-pro-type-reinterpret-cast)
                                  static_cast<size_t>(shader_file_size / sizeof(uint32_t))};

  return device.createShaderModule(vk::ShaderModuleCreateInfo{
      vk::ShaderModuleCreateFlags{},
      shader_code
  });
}

} // namespace

Shader::Shader(const std::filesystem::path &path, vk::Device device)
    : shader_(CreateShaderModule(path, device)), device_(device) {
}

void Shader::AddPushConstantRanges(const std::vector<vk::PushConstantRange> &ranges) {
  push_constant_ranges_.insert(push_constant_ranges_.end(), ranges.begin(), ranges.end());
}

vk::DescriptorSetLayout Shader::AddDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding> &bindings) {
  descriptor_set_layout_bindings_.insert(descriptor_set_layout_bindings_.end(), bindings.begin(), bindings.end());
  vk::DescriptorSetLayout layout = device_.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
      vk::DescriptorSetLayoutCreateFlags{},
      bindings
  });
  descriptor_set_layouts_.push_back(layout);
  return layout;
}

Shader::Shader(Shader &&other) noexcept {
  *this = std::move(other);
}

Shader &Shader::operator=(Shader &&other) noexcept {
  shader_ = other.shader_;
  device_ = other.device_;

  other.shader_ = VK_NULL_HANDLE;
  other.device_ = VK_NULL_HANDLE;

  descriptor_set_layouts_ = std::move(other.descriptor_set_layouts_);
  descriptor_set_layout_bindings_ = std::move(other.descriptor_set_layout_bindings_);
  push_constant_ranges_ = std::move(other.push_constant_ranges_);
  return *this;
}

Shader::~Shader() {
  if (shader_) {
    device_.destroyShaderModule(shader_);
  }
  for (auto &layout : descriptor_set_layouts_) {
    device_.destroyDescriptorSetLayout(layout);
  }
}

} // namespace chove::rendering::vulkan
