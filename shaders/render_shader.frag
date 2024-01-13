out vec4 outColor;

struct DirectionalLight {
    vec3 direction;
    float ambient;

    vec3 color;
    float pad0;
};

struct PointLight {
    float constant;
    float linear;
    float quadratic;
    float near_plane;

    vec3 position;
    float far_plane;

    vec3 color;
    float ambient;

    vec3 positionEyeSpace;
    float pad0;
};

struct SpotLight {
    float constant;
    float linear;
    float quadratic;
    float innerCutoff;

    vec3 position;
    float outerCutoff;

    vec3 direction;
    float ambient;

    vec3 color;
    float pad0;
};

uniform sampler2D ambientTexture;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shininessTexture;
uniform sampler2D alphaTexture;
uniform sampler2D bumpTexture;
uniform sampler2D displacementTexture;

#if DIRECTIONAL_LIGHT_COUNT > 0
uniform sampler2DShadow directionalDepthMaps[DIRECTIONAL_LIGHT_COUNT];
#endif

#if SPOT_LIGHT_COUNT > 0
uniform sampler2DShadow spotDepthMaps[SPOT_LIGHT_COUNT];
#endif

#if POINT_LIGHT_COUNT > 0
uniform samplerCubeShadow pointDepthMaps[POINT_LIGHT_COUNT];
#endif

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
in vec4 fragPosWorld;
in mat3 TBN;

#if DIRECTIONAL_LIGHT_COUNT + SPOT_LIGHT_COUNT > 0
in vec4 fragPosLightSpace[DIRECTIONAL_LIGHT_COUNT + SPOT_LIGHT_COUNT];
#endif

float specularStrength = 0.5f;

vec2 texCoord = vec2(0.0f);

vec3 ambient;
vec3 diffuse;
vec3 specular;

vec3 totalAmbient = vec3(0.0f);
vec3 totalDiffuse = vec3(0.0f);
vec3 totalSpecular = vec3(0.0f);

float directionalDepthBias = 0.005f;
float pointDepthBias = 0.00005f;

void ComputeLightComponents() {
    #ifdef NO_AMBIENT_TEXTURE
        #ifdef NO_DIFFUSE_TEXTURE
            ambient *= ambientColor;
        #else
            ambient *= texture(diffuseTexture, texCoord).xyz * ambientColor;
        #endif
    #else
        ambient *= texture(ambientTexture, texCoord).xyz * ambientColor;
    #endif

    #ifdef NO_DIFFUSE_TEXTURE
        diffuse *= diffuseColor;
    #else
        diffuse *= texture(diffuseTexture, texCoord).xyz * diffuseColor;
    #endif

    #ifdef NO_SPECULAR_TEXTURE
        specular *= specularColor;
    #else
        specular *= texture(specularTexture, texCoord).xyz * specularColor;
    #endif

    totalAmbient += ambient;
    totalDiffuse += diffuse;
    totalSpecular += specular;
}

#if DIRECTIONAL_LIGHT_COUNT > 0
void ComputeDirectionalLight() {
    vec3 cameraPosEye = vec3(0.0f);

    #ifdef NO_BUMP_TEXTURE
        vec3 normalEye = normalize(fragNormal);  // interpolated normals are not normalized
    #else
        vec3 normalEye = normalize(texture(bumpTexture, texCoord).xyz * 2.0 - 1.0);
        normalEye = normalize(inverse(TBN) * normalEye);
    #endif
    vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);  // compute view direction

    for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; ++i) {
        vec3 lightDirN = normalize(directionalLights[i].direction);  // compute light direction
        vec3 halfVector = normalize(lightDirN + viewDirN);  // compute half vector
        ambient = directionalLights[i].ambient * directionalLights[i].color;

        vec3 depthMapCoordinates = ((fragPosLightSpace[i].xyz / fragPosLightSpace[i].w) * 0.5f + 0.5) - vec3(0.0f, 0.0f, directionalDepthBias);

        float totalShadow = 0.0f;
        for (int dx = -2; dx <= 2; ++dx) {
            for (int dy = -2; dy <= 2; ++dy) {
                float shadowCoeff = texture(directionalDepthMaps[i], depthMapCoordinates.xyz + vec3(dx, dy, 0.0f) / 2048.0f);
                totalShadow += shadowCoeff;
            }
        }
        float shadow = totalShadow / 16.0f;

        if (depthMapCoordinates.x < 0.001f || depthMapCoordinates.x > 0.999f || depthMapCoordinates.y < 0.001f || depthMapCoordinates.y > 0.999f) {
            shadow = 0.0f;
        }

        if (depthMapCoordinates.z > 1.0f) {
            shadow = 0.0f;
        }

        diffuse = shadow * max(dot(normalEye, lightDirN), 0.0f) * directionalLights[i].color;

        #ifdef NO_SHININESS_TEXTURE
            float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
        #else
            float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), texture(shininessTexture, texCoord).r);
        #endif
        specular = shadow * specularStrength * specCoeff * directionalLights[i].color;

        ComputeLightComponents();
    }
}
#endif

#if POINT_LIGHT_COUNT > 0

// See: https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap
float GetProjectedDepth(vec3 v, float n, float f)
{
    vec3 absVec = abs(v);
    float lightSpaceZ = max(absVec.x, max(absVec.y, absVec.z));

    float worldSpaceZ = (f + n) / (f - n) - (2 * f * n) / ((f - n) * lightSpaceZ);
    return (worldSpaceZ + 1.0) * 0.5;
}

void ComputePointLight() {
    vec3 cameraPosEye = vec3(0.0f);

    #ifdef NO_BUMP_TEXTURE
        vec3 normalEye = normalize(fragNormal);  // interpolated normals are not normalized
    #else
        vec3 normalEye = normalize(texture(bumpTexture, texCoord).xyz * 2.0 - 1.0);
        normalEye = normalize(inverse(TBN) * normalEye);
    #endif
    vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);  // compute view direction

    for (int i = 0; i < POINT_LIGHT_COUNT; ++i) {
        vec3 lightDirN = normalize(pointLights[i].positionEyeSpace - fragPosEye.xyz);  // compute light direction
        vec3 halfVector = normalize(lightDirN + viewDirN);  // compute half vector
        float dist = length(pointLights[i].positionEyeSpace - fragPosEye.xyz);
        float attenuation = 1.0f / (pointLights[i].constant + pointLights[i].linear * dist + pointLights[i].quadratic * dist * dist);

        vec3 fragToLight = fragPosWorld.xyz - pointLights[i].position;
        float depth = GetProjectedDepth(fragToLight, pointLights[i].near_plane, pointLights[i].far_plane);

        float totalShadow = 0.0f;
        for (int dx = -2; dx <= 2; ++dx) {
            for (int dy = -2; dy <= 2; ++dy) {
                float shadowCoeff = texture(pointDepthMaps[i], vec4(normalize(fragToLight) + vec3(dx, dy, 0.0f) / 2048.0f, depth - pointDepthBias));
                totalShadow += shadowCoeff;
            }
        }
        float shadow = totalShadow / 16.0f;

        ambient = attenuation * pointLights[i].ambient * pointLights[i].color;
        diffuse = shadow * attenuation * max(dot(normalEye, lightDirN), 0.0f) * pointLights[i].color;

        #ifdef NO_SHININESS_TEXTURE
            float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
        #else
            float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), texture(shininessTexture, texCoord).r);
        #endif
        specular = shadow * attenuation * specularStrength * specCoeff * pointLights[i].color;

        ComputeLightComponents();
    }
}
#endif

vec2 ParralaxMapping(vec2 texCoord, vec3 viewDir) {
    float height = texture(displacementTexture, texCoord).r;
    vec2 p = viewDir.xy / viewDir.z * height * 0.05f;
    return texCoord - p;
}

void main() {
    #ifdef NO_DISPLACEMENT_TEXTURE
        texCoord = fragTexCoord;
    #else
        texCoord = ParralaxMapping(fragTexCoord, TBN * normalize(-fragPosEye.xyz));
    #endif

    #if DIRECTIONAL_LIGHT_COUNT > 0
        ComputeDirectionalLight();
    #endif
    #if POINT_LIGHT_COUNT > 0
        ComputePointLight();
    #endif

    float alpha = dissolve;
    #ifndef NO_ALPHA_TEXTURE
        alpha *= texture(alphaTexture, texCoord).r;
    #endif
    if (alpha < 0.01f) discard;
    #ifndef NO_LIGHTS
        outColor = vec4(min(totalAmbient + totalDiffuse + totalSpecular, 1.0f), alpha);
    #else
        #ifdef NO_DIFFUSE_TEXTURE
            outColor = vec4(diffuseColor, 1.0f);
        #else
            outColor = vec4(texture(diffuseTexture, fragTexCoord).xyz, 1.0f);
        #endif
    #endif
}