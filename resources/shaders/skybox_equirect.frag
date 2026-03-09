#version 330 core
in vec3 TexCoords;

out vec4 FragColor;

uniform sampler2D equirectangularMap;
uniform float exposure = 1.0;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(TexCoords));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    // Apply exposure and tone mapping for HDR
    color = vec3(1.0) - exp(-color * exposure);
    
    // Simple gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}
