#version 440

uniform vec4 enabledColor;

out vec4 fragColor;

void main() {
    fragColor = enabledColor;
}
