out vec4 outColor;

uniform sampler2D diffuseTexture;

in vec3 fragNormal;
in vec2 fragTexCoord;

void main() {
    outColor = vec4(texture(diffuseTexture, fragTexCoord).xyz, 1.0f);
}