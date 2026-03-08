#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

out vec4 FragColor;

uniform vec3 uViewPos;

void main()
{
    // Early discard based on distance to avoid expensive calculations
    // Grid plane is 50 units radius from camera, so cull at 60 for smooth fade
    float dist = length(vWorldPos.xz - uViewPos.xz);
    if (dist > 60.0) {
        discard;
    }
    
    // Use world-space XZ coordinates for grid
    vec2 coord = vWorldPos.xz;
    
    // Derivative for anti-aliasing
    vec2 derivative = fwidth(coord);
    
    // Create grid pattern at 1-unit intervals
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    
    // Minimum width for axes
    float minimumz = min(derivative.y, 1.0);
    float minimumx = min(derivative.x, 1.0);
    
    // Grid color (gray)
    vec3 color = vec3(0.2);
    float alpha = 1.0 - min(line, 1.0);
    
    // Major grid lines every 10 units (only calculate if we have some alpha already)
    if (alpha > 0.0) {
        vec2 grid10 = abs(fract(coord / 10.0 - 0.5) - 0.5) / (derivative / 10.0);
        float line10 = min(grid10.x, grid10.y);
        float alpha10 = 1.0 - min(line10, 1.0);
        if (alpha10 > 0.0) {
            color = vec3(0.3);
            alpha = max(alpha, alpha10 * 0.5);
        }
    }
    
    // X axis (red) - along Z direction
    if(abs(vWorldPos.x) < 0.05 + minimumx * 2.0) {
        color = vec3(0.8, 0.1, 0.1);
        alpha = 1.0;
    }
    
    // Z axis (blue) - along X direction  
    if(abs(vWorldPos.z) < 0.05 + minimumz * 2.0) {
        color = vec3(0.1, 0.1, 0.8);
        alpha = 1.0;
    }
    
    // Distance-based fading
    float fade = 1.0 - smoothstep(40.0, 60.0, dist);
    alpha *= fade;
    
    // Discard fully transparent pixels
    if (alpha < 0.01) {
        discard;
    }
    
    FragColor = vec4(color, alpha);
}
