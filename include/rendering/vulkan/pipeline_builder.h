#ifndef CHOVENGINE_INCLUDE_RENDERING_VULKAN_PIPELINE_BUILDER_H_
#define CHOVENGINE_INCLUDE_RENDERING_VULKAN_PIPELINE_BUILDER_H_

#include "rendering/vulkan/shader.h"

#include <vulkan/vulkan.hpp>

namespace chove::rendering::vulkan {
class PipelineBuilder {
 public:
  explicit PipelineBuilder(vk::Device device);

  PipelineBuilder &SetVertexShader(const Shader &shader);
  PipelineBuilder &SetGeometryShader(const Shader &shader);
  PipelineBuilder &SetFragmentShader(const Shader &shader);
  PipelineBuilder &AddInputBufferDescription(const vk::VertexInputBindingDescription &binding_description,
                                             const std::vector<vk::VertexInputAttributeDescription> &attribute_descriptions);
  PipelineBuilder &SetInputTopology(vk::PrimitiveTopology topology);
  PipelineBuilder &SetTessellationControlPointCount(uint32_t count);
  PipelineBuilder &SetViewport(const vk::Viewport &viewport);
  PipelineBuilder &SetScissor(const vk::Rect2D &scissor);
  PipelineBuilder &SetFillMode(vk::PolygonMode mode);
  PipelineBuilder &SetColorBlendEnable(bool enable);
  PipelineBuilder &SetDepthTestEnable(bool enable);

  std::pair<vk::Pipeline, vk::PipelineLayout> build(vk::RenderPass render_pass,
                                                    uint32_t subpass,
                                                    vk::PipelineCreateFlags flags = vk::PipelineCreateFlags{});
 private:
  vk::Device device_;

  std::vector<const Shader*> shaders_;
  std::vector<vk::PipelineShaderStageCreateInfo> shader_stages_;

  std::vector<vk::VertexInputBindingDescription> vertex_input_binding_descriptions_;
  std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions_;

  vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info_;
  vk::PipelineTessellationStateCreateInfo tessellation_state_create_info_{};
  vk::Viewport viewport_;
  vk::Rect2D scissor_;

  vk::PipelineRasterizationStateCreateInfo pipeline_rasterization_state_;
  vk::PipelineColorBlendAttachmentState pipeline_color_attachment_blend_state_;
  vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state_;
  vk::PipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_;
};
} // namespace chove::rendering::vulkan

#endif //CHOVENGINE_INCLUDE_RENDERING_VULKAN_PIPELINE_BUILDER_H_
