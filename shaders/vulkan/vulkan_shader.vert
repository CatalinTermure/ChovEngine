#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 color;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} push;

void main() {
    gl_Position = push.mvp * position;
    color = normal;

    gl_Position.y = -gl_Position.y;
}