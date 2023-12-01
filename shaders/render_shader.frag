out vec4 outColor;

uniform sampler2D diffuseTexture;

layout (std140) uniform Material {
    float shininess;
    float opticalDensity;
    float dissolve;
    vec4 diffuseColor;
    vec4 ambientColor;
    vec4 specularColor;
    vec4 transmissionFilterColor;
};

in vec3 fragNormal;
in vec2 fragTexCoord;

void main() {
    #ifdef NO_DIFFUSE_TEXTURE
    outColor = diffuseColor;
    #else
    outColor = vec4(texture(diffuseTexture, fragTexCoord).xyz, 1.0f);
    #endif
}