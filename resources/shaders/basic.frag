#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

uniform vec3 uViewPos;
uniform vec3 uLightDir;     // direction TO light (normalized)
uniform vec3 uLightColor;
uniform vec3 uObjectColor;

uniform bool uUseTexture;
uniform sampler2D uDiffuseMap;

uniform float uAmbientStrength;
uniform float uSpecularStrength;
uniform float uShininess;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir); // convert direction to light vector

    vec3 ambient = uAmbientStrength * uLightColor;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * uLightColor;

    vec3 V = normalize(uViewPos - vFragPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), uShininess);
    vec3 specular = uSpecularStrength * spec * uLightColor;

    vec3 baseColor = uUseTexture ? texture(uDiffuseMap, vTexCoord).rgb : uObjectColor;
    vec3 color = (ambient + diffuse + specular) * baseColor;

    FragColor = vec4(color, 1.0);
}