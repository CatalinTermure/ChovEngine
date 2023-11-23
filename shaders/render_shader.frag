#version 450

out vec4 outColor;

uniform sampler2D diffuseTexture;

in vec3 fragNormal;
in vec2 fragTexCoord;

void main() {
    outColor = texture(diffuseTexture, fragTexCoord);
}