layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 model;

layout (std140) uniform Matrices {
    mat4 view;
    mat4 projection;
};

out vec3 fragNormal;
out vec2 fragTexCoord;

void main() {
    fragNormal = normal;
    fragTexCoord = texcoord;
    gl_Position = projection * view * model * vec4(position, 1.0f);
}
