#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 color_out;

layout(push_constant) uniform _matrix { mat4 mvp; } matrix;

void main() {
    color_out = color;
    gl_Position = matrix.mvp * position;
}