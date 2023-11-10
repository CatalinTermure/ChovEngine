#include "rendering/vulkan/pipeline_builder.h"

namespace chove::rendering::vulkan {

PipelineBuilder::PipelineBuilder(const Context &context) {
  context_ = &context;
  pipeline_rasterization_state_ = vk::PipelineRasterizationStateCreateInfo{}
      .setLineWidth(1.0f);
  pipeline_color_attachment_blend_state_ = vk::PipelineColorBlendAttachmentState{
      false,
      vk::BlendFactor::eSrcAlpha,
      vk::BlendFactor::eDstAlpha,
      vk::BlendOp::eAdd,
      vk::BlendFactor::eSrcAlpha,
      vk::BlendFactor::eDstAlpha,
      vk::BlendOp::eAdd,
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
          vk::ColorComponentFlagBits::eA
  };
}

PipelineBuilder &PipelineBuilder::SetVertexShader(std::unique_ptr<Shader> shader) {
  vertex_shader_ = std::move(shader);
  vertex_shader_stage_ = vk::PipelineShaderStageCreateInfo{
      vk::PipelineShaderStageCreateFlags{},
      vk::ShaderStageFlagBits::eVertex,
      *vertex_shader_->module(),
      "main"
  };
  return *this;
}

PipelineBuilder &PipelineBuilder::SetFragmentShader(std::unique_ptr<Shader> shader) {
  fragment_shader_ = std::move(shader);
  fragment_shader_stage_ = vk::PipelineShaderStageCreateInfo{
      vk::PipelineShaderStageCreateFlags{},
      vk::ShaderStageFlagBits::eFragment,
      *fragment_shader_->module(),
      "main"
  };
  return *this;
}

std::pair<vk::raii::Pipeline, vk::raii::PipelineLayout> PipelineBuilder::build(vk::Format color_attachment_format, vk::PipelineCreateFlags flags) {
  std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{vertex_shader_stage_, fragment_shader_stage_};
  vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{
      vk::PipelineVertexInputStateCreateFlags{},
      vertex_input_binding_descriptions_,
      vertex_input_attribute_descriptions_
  };
  vk::PipelineViewportStateCreateInfo viewport_state_create_info{
      vk::PipelineViewportStateCreateFlags{},
      viewport_,
      scissor_
  };
  vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state{
      vk::PipelineColorBlendStateCreateFlags{},
      false,
      vk::LogicOp::eNoOp,
      pipeline_color_attachment_blend_state_
  };
  std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
  descriptor_set_layouts.reserve(vertex_shader_->descriptor_set_layouts().size() +
      fragment_shader_->descriptor_set_layouts().size());
  std::vector<vk::PushConstantRange> push_constant_ranges;
  push_constant_ranges.reserve(vertex_shader_->push_constant_ranges().size() +
      fragment_shader_->push_constant_ranges().size());
  push_constant_ranges.insert(push_constant_ranges.end(),
                              vertex_shader_->push_constant_ranges().begin(),
                              vertex_shader_->push_constant_ranges().end());
  push_constant_ranges.insert(push_constant_ranges.end(),
                              fragment_shader_->push_constant_ranges().begin(),
                              fragment_shader_->push_constant_ranges().end());
  for (const auto &descriptor_set_layout : vertex_shader_->descriptor_set_layouts()) {
    descriptor_set_layouts.push_back(*descriptor_set_layout);
  }
  for (const auto &descriptor_set_layout : fragment_shader_->descriptor_set_layouts()) {
    descriptor_set_layouts.push_back(*descriptor_set_layout);
  }
  vk::raii::PipelineLayout layout{context_->device(),
                                  vk::PipelineLayoutCreateInfo{
                                      vk::PipelineLayoutCreateFlags{},
                                      descriptor_set_layouts,
                                      push_constant_ranges
                                  }};

  vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{
      0,
      color_attachment_format,
      vk::Format::eUndefined,
      vk::Format::eUndefined
  };
  return {vk::raii::Pipeline{context_->device(), nullptr, vk::GraphicsPipelineCreateInfo{
      flags,
      shader_stages,
      &vertex_input_state_create_info,
      &input_assembly_state_create_info_,
      &tessellation_state_create_info_,
      &viewport_state_create_info,
      &pipeline_rasterization_state_,
      &pipeline_multisample_state_,
      nullptr,
      &pipeline_color_blend_state,
      nullptr,
      *layout,
      nullptr,
      0,
      nullptr, // TODO: Try to use pipeline derivatives, should be simple with the builder pattern to cache the last pipeline built.
      0,
      &pipeline_rendering_create_info
  }}, std::move(layout)};
}

PipelineBuilder &PipelineBuilder::AddInputBufferDescription(const vk::VertexInputBindingDescription &binding_description,
                                                            const std::vector<vk::VertexInputAttributeDescription> &attribute_descriptions) {
  vertex_input_binding_descriptions_.push_back(binding_description);
  vertex_input_attribute_descriptions_.insert(vertex_input_attribute_descriptions_.end(),
                                              attribute_descriptions.begin(),
                                              attribute_descriptions.end());
  return *this;
}

PipelineBuilder &PipelineBuilder::SetInputTopology(vk::PrimitiveTopology topology) {
  input_assembly_state_create_info_.topology = topology;
  return *this;
}
PipelineBuilder &PipelineBuilder::SetTessellationControlPointCount(uint32_t count) {
  tessellation_state_create_info_.patchControlPoints = count;
  return *this;
}
PipelineBuilder &PipelineBuilder::SetViewport(const vk::Viewport &viewport) {
  viewport_ = viewport;
  return *this;
}
PipelineBuilder &PipelineBuilder::SetScissor(const vk::Rect2D &scissor) {
  scissor_ = scissor;
  return *this;
}
PipelineBuilder &PipelineBuilder::SetFillMode(vk::PolygonMode mode) {
  pipeline_rasterization_state_.polygonMode = mode;
  return *this;
}
PipelineBuilder &PipelineBuilder::SetColorBlendEnable(bool enable) {
  pipeline_color_attachment_blend_state_.blendEnable = enable;
  return *this;
}

}
