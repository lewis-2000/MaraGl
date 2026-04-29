#version 460 core

out vec4 FragColor;

uniform vec3 uColor;
uniform float uIntensity;

void main()
{
    // Create glowing effect by multiplying color by intensity
    vec3 glowColor = uColor * uIntensity;
    FragColor = vec4(glowColor, 1.0);
}
