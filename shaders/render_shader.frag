#version 450

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Size { vec2 size; } windowSize;

void main() {
    outColor = vec4(gl_FragCoord.x / windowSize.size.x, gl_FragCoord.y / windowSize.size.y, 0.5f, 1.0);
}