#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

layout(binding = 0) uniform MaterialData {
    float shininess;
    float optical_density;
    float dissolve;
    vec3 transmission_filter_color;
    vec3 ambient_color;
    vec3 diffuse_color;
    vec3 specular_color;
} material_data;

const int ambient_texture = 0;
const int diffuse_texture = 1;
const int specular_texture = 2;
const int specular_highlight_texture = 3;
const int alpha_texture = 4;
const int bump_texture = 5;
const int displacement_texture = 6;

layout(binding = 1) uniform sampler2D textures[7];

layout(location = 0) out vec4 color_out;

layout(push_constant) uniform _matrix { mat4 mvp; } matrix;

void main() {
    color_out = vec4(normal, 1.0f);
    gl_Position = matrix.mvp * vec4(position, 1.0f);
}