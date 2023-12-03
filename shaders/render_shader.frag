out vec4 outColor;

uniform sampler2D diffuseTexture;

layout (std140) uniform Material {
    float shininess;
    float opticalDensity;
    float dissolve;
    vec3 diffuseColor;
    vec3 ambientColor;
    vec3 specularColor;
    vec3 transmissionFilterColor;
};

in vec3 fragNormal;
in vec2 fragTexCoord;

void main() {
    #ifdef NO_DIFFUSE_TEXTURE
    outColor = vec4(diffuseColor, 1.0f);
    #else
    outColor = vec4(texture(diffuseTexture, fragTexCoord).xyz, 1.0f);
    #endif
}