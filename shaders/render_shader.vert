#define MAX_DEPTH_MAPS 8

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec3 tangent;

uniform mat4 model;
uniform mat3 normalMatrix;

layout (std140) uniform Matrices {
    mat4 view;
    mat4 projection;
};

layout (std140) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[MAX_DEPTH_MAPS];
};

out vec3 fragNormal;
out vec2 fragTexCoord;
out vec4 fragPosEye;
out vec4 fragPosWorld;
out mat3 TBN;

void main() {
    fragPosEye = view * model * vec4(position, 1.0f);
    fragPosWorld = model * vec4(position, 1.0f);
    fragNormal = normalize(normalMatrix * normal);
    fragTexCoord = texcoord;

    vec3 T = normalize((view * model * vec4(tangent, 0.0f)).xyz);
    vec3 N = normalize((view * model * vec4(normal, 0.0f)).xyz);
    vec3 B = cross(N, T);

    TBN = transpose(mat3(T, B, N));

    gl_Position = projection * view * model * vec4(position, 1.0f);
}
