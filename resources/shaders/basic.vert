#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aBoneWeights;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Animation uniforms
// Keep this conservative for GL 3.3 uniform limits.
const int MAX_BONES = 100;
uniform bool uUseAnimation;
uniform mat4 uBoneTransforms[MAX_BONES];

void main()
{
    vec4 localPos;
    vec3 localNormal;

    if (uUseAnimation)
    {
        // Calculate skinned position with weight normalization
        mat4 boneTransform = mat4(0.0);
        float totalWeight = 0.0;
        
        for (int i = 0; i < 4; i++)
        {
            if (aBoneIDs[i] >= 0 && aBoneIDs[i] < MAX_BONES)
            {
                boneTransform += uBoneTransforms[aBoneIDs[i]] * aBoneWeights[i];
                totalWeight += aBoneWeights[i];
            }
        }
        
        // If this vertex has no bone weights, use identity transform
        if (totalWeight < 0.01)
        {
            localPos = vec4(aPos, 1.0);
            localNormal = aNormal;
        }
        else
        {
            localPos = boneTransform * vec4(aPos, 1.0);
            localNormal = mat3(boneTransform) * aNormal;
        }
    }
    else
    {
        localPos = vec4(aPos, 1.0);
        localNormal = aNormal;
    }

    vec4 worldPos = model * localPos;
    vFragPos = worldPos.xyz;

    // Correct normal transform for scaled models
    vNormal = mat3(transpose(inverse(model))) * localNormal;
    vTexCoord = aTexCoord;

    gl_Position = projection * view * worldPos;
}