layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

out vec2 fragTexCoord;

void main() {
    fragTexCoord = texCoord;
    gl_Position = lightSpaceMatrix * model * vec4(position, 1.0);
}
