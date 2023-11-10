#include "rendering/vulkan/shader.h"

#include <fstream>

namespace chove::rendering::vulkan {

namespace {

vk::raii::ShaderModule GetShaderModule(const std::filesystem::path &shader_file_path, const vk::raii::Device &device) {
  std::ifstream shader_file{shader_file_path, std::ios::binary};
  shader_file.seekg(0, std::ios::end);
  auto shader_file_size = shader_file.tellg();
  shader_file.seekg(0, std::ios::beg);
  std::vector<char> buffer(shader_file_size);
  shader_file.read(buffer.data(), shader_file_size);
  std::vector<uint32_t> shader_code(reinterpret_cast<uint32_t *>(buffer.data()),
                                    reinterpret_cast<uint32_t *>(buffer.data() + shader_file_size));

  return vk::raii::ShaderModule{device, vk::ShaderModuleCreateInfo{
      vk::ShaderModuleCreateFlags{},
      shader_code
  }};
}
}

Shader::Shader(const std::filesystem::path& path, const Context &context)
    : shader_(GetShaderModule(path, context.device())), context_(&context) {
}

void Shader::AddPushConstantRanges(const std::vector<vk::PushConstantRange> &ranges) {
  push_constant_ranges_.insert(push_constant_ranges_.end(), ranges.begin(), ranges.end());
}

void Shader::AddDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding> &bindings) {
  descriptor_set_layout_bindings_.insert(descriptor_set_layout_bindings_.end(), bindings.begin(), bindings.end());
  descriptor_set_layouts_.emplace_back(context_->device(), vk::DescriptorSetLayoutCreateInfo{
      vk::DescriptorSetLayoutCreateFlags{},
      bindings
  });
}

}
