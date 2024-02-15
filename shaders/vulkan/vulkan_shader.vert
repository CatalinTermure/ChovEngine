#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout (binding = 0, set = 0) uniform _matrices {
    mat4 view;
    mat4 projection;
} matrices;

layout(location = 0) out vec4 color_out;

layout(push_constant) uniform _model { mat4 matrix; } model;

void main() {
    color_out = vec4(normal, 1.0f);
    gl_Position = matrices.projection * matrices.view * model.matrix * vec4(position, 1.0f);
}