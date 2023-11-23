#include "rendering/vulkan/shader.h"
#include "rendering/opengl/shader.h"

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

Shader::Shader(const std::filesystem::path &path, const Context &context)
    : shader_(GetShaderModule(path, context.device())), context_(&context) {
}

void Shader::AddPushConstantRanges(const std::vector<vk::PushConstantRange> &ranges) {
  push_constant_ranges_.insert(push_constant_ranges_.end(), ranges.begin(), ranges.end());
}

vk::DescriptorSetLayout Shader::AddDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding> &bindings) {
  descriptor_set_layout_bindings_.insert(descriptor_set_layout_bindings_.end(), bindings.begin(), bindings.end());

  std::vector<VkDescriptorSetLayoutBinding> bindings_c;
  for (const auto &binding : bindings) {
    bindings_c.push_back({
                             .binding = binding.binding,
                             .descriptorType = static_cast<VkDescriptorType>(binding.descriptorType),
                             .descriptorCount = binding.descriptorCount,
                             .stageFlags = static_cast<VkShaderStageFlags>(binding.stageFlags),
                             .pImmutableSamplers = nullptr
                         });
  }

  VkDescriptorSetLayoutCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = static_cast<uint32_t>(bindings_c.size()),
      .pBindings = bindings_c.data()
  };
  VkDescriptorSetLayout layout;
  VkResult result = vkCreateDescriptorSetLayout(*context_->device(), &create_info, nullptr, &layout);
  assert(result == VK_SUCCESS);
  descriptor_set_layouts_.emplace_back(context_->device(), layout);
  return vk::DescriptorSetLayout{layout};
}
Shader::Shader(Shader &&other) : shader_(std::move(other.shader_)),
                                 descriptor_set_layouts_(std::move(other.descriptor_set_layouts_)),
                                 descriptor_set_layout_bindings_(std::move(other.descriptor_set_layout_bindings_)),
                                 push_constant_ranges_(std::move(other.push_constant_ranges_)) {
  other.context_ = nullptr;
}
Shader &Shader::operator=(Shader &&other) {
  shader_ = std::move(other.shader_);
  descriptor_set_layouts_ = std::move(other.descriptor_set_layouts_);
  descriptor_set_layout_bindings_ = std::move(other.descriptor_set_layout_bindings_);
  push_constant_ranges_ = std::move(other.push_constant_ranges_);
  context_ = other.context_;
  other.context_ = nullptr;
  return *this;
}

}
