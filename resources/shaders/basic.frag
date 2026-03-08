#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

uniform vec3 uViewPos;
uniform vec3 uObjectColor;

uniform bool uUseTexture;
uniform sampler2D uDiffuseMap;

uniform float uAmbientStrength;
uniform float uSpecularStrength;
uniform float uShininess;

// Overcast light (global ambient illumination)
uniform bool uOvercastEnabled;
uniform vec3 uOvercastColor;
uniform float uOvercastIntensity;

// Directional Light
struct DirLight
{
    vec3 direction;
    vec3 color;
    float intensity;
};

uniform DirLight uDirLight;
uniform int uPointLightCount;
uniform int uSpotLightCount;

// Point Light
struct PointLight
{
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

const int MAX_POINT_LIGHTS = 4;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];

// Spot Light
struct SpotLight
{
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float innerCutoff;
    float outerCutoff;
};

const int MAX_SPOT_LIGHTS = 2;
uniform SpotLight uSpotLights[MAX_SPOT_LIGHTS];

vec3 CalcDirLight(DirLight light, vec3 norm, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Ambient
    vec3 ambient = uAmbientStrength * light.color * light.intensity;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color * light.intensity;
    
    // Specular
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), uShininess);
    vec3 specular = uSpecularStrength * spec * light.color * light.intensity;
    
    return (ambient + diffuse + specular) * baseColor;
}

vec3 CalcPointLight(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    
    // Attenuation with smooth falloff
    float attenuation = 1.0 / (1.0 + (distance / light.range) * (distance / light.range));
    
    // Ambient
    vec3 ambient = uAmbientStrength * light.color * light.intensity * attenuation;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color * light.intensity * attenuation;
    
    // Specular
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), uShininess);
    vec3 specular = uSpecularStrength * spec * light.color * light.intensity * attenuation;
    
    return (ambient + diffuse + specular) * baseColor;
}

vec3 CalcSpotLight(SpotLight light, vec3 norm, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    
    // Attenuation
    float attenuation = 1.0 / (1.0 + (distance / light.range) * (distance / light.range));
    
    // Spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.innerCutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
    
    // Ambient
    vec3 ambient = uAmbientStrength * light.color * light.intensity * attenuation * intensity;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color * light.intensity * attenuation * intensity;
    
    // Specular
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), uShininess);
    vec3 specular = uSpecularStrength * spec * light.color * light.intensity * attenuation * intensity;
    
    return (ambient + diffuse + specular) * baseColor;
}

void main()
{
    vec3 basColor = uUseTexture ? texture(uDiffuseMap, vTexCoord).rgb : uObjectColor;
    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(uViewPos - vFragPos);
    
    vec3 result = vec3(0.0);
    
    // Overcast light (uniform ambient illumination)
    if (uOvercastEnabled)
    {
        result += basColor * uOvercastColor * uOvercastIntensity;
    }
    
    // Directional Light
    if (uDirLight.intensity > 0.0)
    {
        result += CalcDirLight(uDirLight, norm, viewDir, basColor);
    }
    
    // Point Lights
    for (int i = 0; i < uPointLightCount; i++)
    {
        result += CalcPointLight(uPointLights[i], norm, vFragPos, viewDir, basColor);
    }
    
    // Spot Lights
    for (int i = 0; i < uSpotLightCount; i++)
    {
        result += CalcSpotLight(uSpotLights[i], norm, vFragPos, viewDir, basColor);
    }
    
    FragColor = vec4(result, 1.0);
}