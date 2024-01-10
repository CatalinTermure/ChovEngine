out vec4 outColor;

uniform float dissolve;
uniform sampler2D alphaTexture;

in vec2 fragTexCoord;

void main() {
    float alpha = dissolve * texture(alphaTexture, fragTexCoord).r;
    if (alpha < 0.99f) {
        discard;
    }
}
