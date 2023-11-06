#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 inColor;

layout(push_constant) uniform Size { vec2 size; } windowSize;

void main() {
    outColor = inColor;
}