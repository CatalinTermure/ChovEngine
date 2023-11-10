#ifndef LABSEXTRA_INCLUDE_RENDERING_VULKAN_PIPELINE_BUILDER_H_
#define LABSEXTRA_INCLUDE_RENDERING_VULKAN_PIPELINE_BUILDER_H_

#include <vulkan/vulkan_raii.hpp>

#include "shader.h"

namespace chove::rendering::vulkan {
class PipelineBuilder {
 public:
  explicit PipelineBuilder(const Context &context);

  PipelineBuilder &SetVertexShader(std::unique_ptr<Shader> shader);
  PipelineBuilder &SetFragmentShader(std::unique_ptr<Shader> shader);
  PipelineBuilder &AddInputBufferDescription(const vk::VertexInputBindingDescription &binding_description,
                                             const std::vector<vk::VertexInputAttributeDescription> &attribute_descriptions);
  PipelineBuilder &SetInputTopology(vk::PrimitiveTopology topology);
  PipelineBuilder &SetTessellationControlPointCount(uint32_t count);
  PipelineBuilder &SetViewport(const vk::Viewport &viewport);
  PipelineBuilder &SetScissor(const vk::Rect2D &scissor);
  PipelineBuilder &SetFillMode(vk::PolygonMode mode);
  PipelineBuilder &SetColorBlendEnable(bool enable);

  std::pair<vk::raii::Pipeline, vk::raii::PipelineLayout> build(vk::Format color_attachment_format,
                           vk::PipelineCreateFlags flags = vk::PipelineCreateFlags{});
 private:
  const Context *context_;

  std::unique_ptr<Shader> vertex_shader_;
  std::unique_ptr<Shader> fragment_shader_;
  vk::PipelineShaderStageCreateInfo vertex_shader_stage_;
  vk::PipelineShaderStageCreateInfo fragment_shader_stage_;

  std::vector<vk::VertexInputBindingDescription> vertex_input_binding_descriptions_;
  std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions_;

  vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info_;
  vk::PipelineTessellationStateCreateInfo tessellation_state_create_info_;
  vk::Viewport viewport_;
  vk::Rect2D scissor_;

  vk::PipelineRasterizationStateCreateInfo pipeline_rasterization_state_;
  vk::PipelineColorBlendAttachmentState pipeline_color_attachment_blend_state_;
  vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state_;
};
} // namespace chove::rendering

#endif //LABSEXTRA_INCLUDE_RENDERING_VULKAN_PIPELINE_BUILDER_H_
