#version 450

layout(location = 0) in vec4 position;

layout(push_constant) uniform Camera { layout(offset = 16) mat4 cameraMatrix; } camera;

void main() {
    vec4 newPoint = camera.cameraMatrix * position;
    gl_Position = newPoint / newPoint.w;
}