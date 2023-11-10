#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 color_out;

layout(push_constant) uniform Camera { mat4 cameraMatrix; } camera;

void main() {
    vec4 newPoint = camera.cameraMatrix * position;
    gl_Position = newPoint / newPoint.w;
    color_out = color;
}