out vec4 outColor;

struct DirectionalLight {
    vec3 direction;
    float pad0;

    vec3 color;
    float pad1;
};

struct PointLight {
    float constant;
    float linear;
    float quadratic;
    float pad0;

    vec3 position;
    float pad1;

    vec3 color;
    float pad2;
};

struct SpotLight {
    float constant;
    float linear;
    float quadratic;
    float innerCutoff;

    vec3 position;
    float outerCutoff;

    vec3 direction;
    float pad0;

    vec3 color;
    float pad1;
};

uniform sampler2D ambientTexture;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shininessTexture;
uniform sampler2D alphaTexture;
uniform sampler2D bumpTexture;
uniform sampler2D displacementTexture;

layout (std140) uniform Material {
    float shininess;
    float opticalDensity;
    float dissolve;
    vec3 diffuseColor;
    vec3 ambientColor;
    vec3 specularColor;
    vec3 transmissionFilterColor;
};

layout (std140) uniform Lights {
#if DIRECTIONAL_LIGHT_COUNT > 0
    DirectionalLight directionalLights[DIRECTIONAL_LIGHT_COUNT];
#endif
#if POINT_LIGHT_COUNT > 0
    PointLight pointLights[POINT_LIGHT_COUNT];
#endif
#if SPOT_LIGHT_COUNT > 0
    SpotLight spotLights[SPOT_LIGHT_COUNT];
#endif
};

in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragPosEye;

float ambientStrength = 0.2f;
float specularStrength = 0.5f;

vec3 ambient;
vec3 diffuse;
vec3 specular;

vec3 totalAmbient = vec3(0.0f);
vec3 totalDiffuse = vec3(0.0f);
vec3 totalSpecular = vec3(0.0f);


void ComputeLightComponents() {
    #ifdef NO_AMBIENT_TEXTURE
        #ifdef NO_DIFFUSE_TEXTURE
            ambient *= ambientColor;
        #else
            ambient *= texture(diffuseTexture, fragTexCoord).xyz * ambientColor;
        #endif
    #else
        ambient *= texture(ambientTexture, fragTexCoord).xyz * ambientColor;
    #endif

    #ifdef NO_DIFFUSE_TEXTURE
        diffuse *= diffuseColor;
    #else
        diffuse *= texture(diffuseTexture, fragTexCoord).xyz * diffuseColor;
    #endif

    #ifdef NO_SPECULAR_TEXTURE
        specular *= specularColor;
    #else
        specular *= texture(specularTexture, fragTexCoord).xyz * specularColor;
    #endif

    totalAmbient += ambient;
    totalDiffuse += diffuse;
    totalSpecular += specular;
}

void ComputeDirectionalLight() {
    vec3 cameraPosEye = vec3(0.0f);

    vec3 normalEye = normalize(fragNormal);// interpolated normals are not normalized
    vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);// compute view direction

    for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; ++i) {
        vec3 lightDirN = normalize(directionalLights[i].direction - fragPosEye.xyz);// compute light direction
        vec3 halfVector = normalize(lightDirN + viewDirN);// compute half vector
        ambient = ambientStrength * directionalLights[i].color;
        diffuse = max(dot(normalEye, lightDirN), 0.0f) * directionalLights[i].color;

        #ifdef NO_SHININESS_TEXTURE
            float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
        #else
            float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), texture(shininessTexture, fragTexCoord).r);
        #endif
        specular = specularStrength * specCoeff * directionalLights[i].color;

        ComputeLightComponents();
    }
}

void ComputePointLight() {
    vec3 cameraPosEye = vec3(0.0f);

    vec3 normalEye = normalize(fragNormal);// interpolated normals are not normalized
    vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);// compute view direction

    for (int i = 0; i < POINT_LIGHT_COUNT; ++i) {
        vec3 lightDirN = normalize(pointLights[i].position - fragPosEye.xyz);// compute light direction
        vec3 halfVector = normalize(lightDirN + viewDirN);// compute half vector
        float distance = length(pointLights[i].position - fragPosEye.xyz);
        float attenuation = 1.0f / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * distance * distance);

        ambient = attenuation * ambientStrength * pointLights[i].color;
        diffuse = attenuation * max(dot(normalEye, lightDirN), 0.0f) * pointLights[i].color;

        #ifdef NO_SHININESS_TEXTURE
            float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
        #else
            float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), texture(shininessTexture, fragTexCoord).r);
        #endif
        specular = attenuation * specularStrength * specCoeff * pointLights[i].color;

        ComputeLightComponents();
    }
}

void main() {
    ComputePointLight();
    ComputeDirectionalLight();
    #ifndef NO_LIGHTS
        outColor = vec4(min(totalAmbient + totalDiffuse + totalSpecular, 1.0f), 1.0f);
    #else
        #ifdef NO_DIFFUSE_TEXTURE
            outColor = vec4(diffuseColor, 1.0f);
        #else
            outColor = vec4(texture(diffuseTexture, fragTexCoord).xyz, 1.0f);
        #endif
    #endif
}