#include "rendering/vulkan/pipeline_builder.h"

namespace chove::rendering::vulkan {

PipelineBuilder::PipelineBuilder(const vk::Device device) : device_(device) {
  pipeline_rasterization_state_ = vk::PipelineRasterizationStateCreateInfo{
      vk::PipelineRasterizationStateCreateFlags{},
      false,
      false,
      vk::PolygonMode::eFill,
      vk::CullModeFlagBits::eNone,
      vk::FrontFace::eCounterClockwise,
      false,
      0.0F,
      0.0F,
      0.0F,
      1.0F
  };
  pipeline_color_attachment_blend_state_ = vk::PipelineColorBlendAttachmentState{
      false,
      vk::BlendFactor::eSrcAlpha,
      vk::BlendFactor::eOneMinusSrcAlpha,
      vk::BlendOp::eAdd,
      vk::BlendFactor::eSrcAlpha,
      vk::BlendFactor::eOneMinusSrcAlpha,
      vk::BlendOp::eAdd,
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
          vk::ColorComponentFlagBits::eA
  };
  input_assembly_state_create_info_ = vk::PipelineInputAssemblyStateCreateInfo{
      vk::PipelineInputAssemblyStateCreateFlags{}, vk::PrimitiveTopology::eTriangleList, false
  };
  pipeline_depth_stencil_state_ = vk::PipelineDepthStencilStateCreateInfo{
      vk::PipelineDepthStencilStateCreateFlags{},
      true,
      true,
      vk::CompareOp::eLess,
      false,
      false,
      vk::StencilOpState{},
      vk::StencilOpState{},
      0.0F,
      1.0F
  };
}

PipelineBuilder &PipelineBuilder::SetVertexShader(const Shader &shader) {
  shader_stages_.emplace_back(
      vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eVertex, shader.module(), "main"
  );
  shaders_.push_back(&shader);
  return *this;
}

PipelineBuilder &PipelineBuilder::SetGeometryShader(const Shader &shader) {
  shader_stages_.emplace_back(
      vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eGeometry, shader.module(), "main"
  );
  shaders_.push_back(&shader);
  return *this;
}

PipelineBuilder &PipelineBuilder::SetFragmentShader(const Shader &shader) {
  shader_stages_.emplace_back(
      vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, shader.module(), "main"
  );
  shaders_.push_back(&shader);
  return *this;
}

std::pair<vk::Pipeline, vk::PipelineLayout> PipelineBuilder::build(
    const vk::RenderPass render_pass, const uint32_t subpass, const vk::PipelineCreateFlags flags
) {
  const vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{
      vk::PipelineVertexInputStateCreateFlags{},
      vertex_input_binding_descriptions_,
      vertex_input_attribute_descriptions_
  };
  const vk::PipelineViewportStateCreateInfo viewport_state_create_info{
      vk::PipelineViewportStateCreateFlags{}, viewport_, scissor_
  };
  const vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state{
      vk::PipelineColorBlendStateCreateFlags{}, false, vk::LogicOp::eNoOp, pipeline_color_attachment_blend_state_
  };
  std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
  std::vector<vk::PushConstantRange> push_constant_ranges;
  for (const auto &shader : shaders_) {
    descriptor_set_layouts.insert(
        descriptor_set_layouts.end(), shader->descriptor_set_layouts().begin(), shader->descriptor_set_layouts().end()
    );
    push_constant_ranges.insert(
        push_constant_ranges.end(), shader->push_constant_ranges().begin(), shader->push_constant_ranges().end()
    );
  }
  vk::PipelineLayout layout = device_.createPipelineLayout(
      vk::PipelineLayoutCreateInfo{vk::PipelineLayoutCreateFlags{}, descriptor_set_layouts, push_constant_ranges}
  );

  vk::ResultValue<vk::Pipeline> result = device_.createGraphicsPipeline(
      nullptr,
      vk::GraphicsPipelineCreateInfo{
          flags,
          shader_stages_,
          &vertex_input_state_create_info,
          &input_assembly_state_create_info_,
          &tessellation_state_create_info_,
          &viewport_state_create_info,
          &pipeline_rasterization_state_,
          &pipeline_multisample_state_,
          &pipeline_depth_stencil_state_,
          &pipeline_color_blend_state,
          nullptr,
          layout,
          render_pass,
          subpass,
          nullptr,  // TODO: Try to use pipeline derivatives
          0,
          nullptr
      }
  );

  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create pipeline.");
  }

  return {result.value, layout};
}

PipelineBuilder &PipelineBuilder::AddInputBufferDescription(
    const vk::VertexInputBindingDescription &binding_description,
    const std::vector<vk::VertexInputAttributeDescription> &attribute_descriptions
) {
  vertex_input_binding_descriptions_.push_back(binding_description);
  vertex_input_attribute_descriptions_.insert(
      vertex_input_attribute_descriptions_.end(), attribute_descriptions.begin(), attribute_descriptions.end()
  );
  return *this;
}

PipelineBuilder &PipelineBuilder::SetInputTopology(const vk::PrimitiveTopology topology) {
  input_assembly_state_create_info_.topology = topology;
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

PipelineBuilder &PipelineBuilder::SetFillMode(const vk::PolygonMode mode) {
  pipeline_rasterization_state_.polygonMode = mode;
  return *this;
}

PipelineBuilder &PipelineBuilder::SetColorBlendEnable(const bool enable) {
  pipeline_color_attachment_blend_state_.blendEnable = enable;
  return *this;
}

PipelineBuilder &PipelineBuilder::SetDepthTestEnable(const bool enable) {
  pipeline_depth_stencil_state_.depthTestEnable = enable;
  pipeline_depth_stencil_state_.depthWriteEnable = enable;
  return *this;
}

}  // namespace chove::rendering::vulkan
