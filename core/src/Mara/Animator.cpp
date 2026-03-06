#include "Animator.h"
#include "AnimationComponent.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace MaraGl
{
    void Animator::UpdateAnimation(AnimationComponent *animComp, float deltaTime)
    {
        if (!animComp || animComp->animations.empty() || !animComp->playing)
            return;

        Animation &currentAnim = animComp->animations[animComp->currentAnimation];

        // Update animation time
        animComp->currentTime += currentAnim.ticksPerSecond * deltaTime * animComp->playbackSpeed;

        float duration = currentAnim.duration;
        if (animComp->looping)
        {
            animComp->currentTime = fmod(animComp->currentTime, duration);
        }
        else
        {
            if (animComp->currentTime >= duration)
            {
                animComp->currentTime = duration;
                animComp->playing = false;
            }
        }

        // Debug output every 60 frames (~1 second)
        static int frameCount = 0;
        if (++frameCount % 60 == 0)
        {
            std::cout << "[Animator] Playing: " << currentAnim.name
                      << " | Time: " << animComp->currentTime << "/" << duration
                      << " | Channels: " << currentAnim.boneAnimations.size() << std::endl;
        }
    }

    void Animator::CalculateBoneTransform(AnimationComponent *animComp,
                                          const Animation &animation,
                                          const aiNode *node,
                                          const glm::mat4 &parentTransform)
    {
        if (!node)
            return;

        std::string nodeName = node->mName.C_Str();
        glm::mat4 nodeTransform = ConvertMatrixToGLM(node->mTransformation);

        // Find animation channel for this node
        const BoneAnimation *boneAnim = FindBoneAnimation(animation, nodeName);

        if (boneAnim)
        {
            // Interpolate position, rotation, and scale
            glm::mat4 translation = InterpolatePosition(*boneAnim, animComp->currentTime);
            glm::mat4 rotation = InterpolateRotation(*boneAnim, animComp->currentTime);
            glm::mat4 scale = InterpolateScale(*boneAnim, animComp->currentTime);

            nodeTransform = translation * rotation * scale;
        }

        glm::mat4 globalTransform = parentTransform * nodeTransform;

        // If this node is a bone, update its final transform
        auto it = animComp->boneInfoMap.find(nodeName);
        if (it != animComp->boneInfoMap.end())
        {
            int boneIndex = it->second.id;
            glm::mat4 offset = it->second.offset;
            animComp->boneTransforms[boneIndex] = globalTransform * offset;
        }

        // Recursively process all children (even those without animation channels)
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            CalculateBoneTransform(animComp, animation, node->mChildren[i], globalTransform);
        }
    }

    int Animator::FindPositionIndex(const BoneAnimation &boneAnim, float animationTime)
    {
        for (size_t i = 0; i + 1 < boneAnim.positions.size(); i++)
        {
            if (animationTime < boneAnim.positions[i + 1].timeStamp)
                return static_cast<int>(i);
        }

        return static_cast<int>(boneAnim.positions.size() - 2);
    }

    int Animator::FindRotationIndex(const BoneAnimation &boneAnim, float animationTime)
    {
        for (size_t i = 0; i + 1 < boneAnim.rotations.size(); i++)
        {
            if (animationTime < boneAnim.rotations[i + 1].timeStamp)
                return static_cast<int>(i);
        }

        return static_cast<int>(boneAnim.rotations.size() - 2);
    }

    int Animator::FindScaleIndex(const BoneAnimation &boneAnim, float animationTime)
    {
        for (size_t i = 0; i + 1 < boneAnim.scales.size(); i++)
        {
            if (animationTime < boneAnim.scales[i + 1].timeStamp)
                return static_cast<int>(i);
        }

        return static_cast<int>(boneAnim.scales.size() - 2);
    }

    float Animator::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
    {
        float midwayLength = animationTime - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        return midwayLength / framesDiff;
    }

    glm::mat4 Animator::InterpolatePosition(const BoneAnimation &boneAnim, float animationTime)
    {
        if (boneAnim.positions.size() == 1)
            return glm::translate(glm::mat4(1.0f), boneAnim.positions[0].position);

        int p0Index = FindPositionIndex(boneAnim, animationTime);
        int p1Index = p0Index + 1;

        float scaleFactor = GetScaleFactor(boneAnim.positions[p0Index].timeStamp,
                                           boneAnim.positions[p1Index].timeStamp,
                                           animationTime);

        glm::vec3 finalPosition = glm::mix(boneAnim.positions[p0Index].position,
                                           boneAnim.positions[p1Index].position,
                                           scaleFactor);

        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    glm::mat4 Animator::InterpolateRotation(const BoneAnimation &boneAnim, float animationTime)
    {
        if (boneAnim.rotations.size() == 1)
            return glm::mat4_cast(boneAnim.rotations[0].orientation);

        int p0Index = FindRotationIndex(boneAnim, animationTime);
        int p1Index = p0Index + 1;

        float scaleFactor = GetScaleFactor(boneAnim.rotations[p0Index].timeStamp,
                                           boneAnim.rotations[p1Index].timeStamp,
                                           animationTime);

        glm::quat finalRotation = glm::slerp(boneAnim.rotations[p0Index].orientation,
                                             boneAnim.rotations[p1Index].orientation,
                                             scaleFactor);
        finalRotation = glm::normalize(finalRotation);

        return glm::mat4_cast(finalRotation);
    }

    glm::mat4 Animator::InterpolateScale(const BoneAnimation &boneAnim, float animationTime)
    {
        if (boneAnim.scales.size() == 1)
            return glm::scale(glm::mat4(1.0f), boneAnim.scales[0].scale);

        int p0Index = FindScaleIndex(boneAnim, animationTime);
        int p1Index = p0Index + 1;

        float scaleFactor = GetScaleFactor(boneAnim.scales[p0Index].timeStamp,
                                           boneAnim.scales[p1Index].timeStamp,
                                           animationTime);

        glm::vec3 finalScale = glm::mix(boneAnim.scales[p0Index].scale,
                                        boneAnim.scales[p1Index].scale,
                                        scaleFactor);

        return glm::scale(glm::mat4(1.0f), finalScale);
    }

    const BoneAnimation *Animator::FindBoneAnimation(const Animation &animation, const std::string &nodeName)
    {
        for (const auto &boneAnim : animation.boneAnimations)
        {
            if (boneAnim.boneName == nodeName)
                return &boneAnim;
        }
        return nullptr;
    }

    glm::mat4 Animator::ConvertMatrixToGLM(const aiMatrix4x4 &from)
    {
        glm::mat4 to;
        to[0][0] = from.a1;
        to[1][0] = from.a2;
        to[2][0] = from.a3;
        to[3][0] = from.a4;
        to[0][1] = from.b1;
        to[1][1] = from.b2;
        to[2][1] = from.b3;
        to[3][1] = from.b4;
        to[0][2] = from.c1;
        to[1][2] = from.c2;
        to[2][2] = from.c3;
        to[3][2] = from.c4;
        to[0][3] = from.d1;
        to[1][3] = from.d2;
        to[2][3] = from.d3;
        to[3][3] = from.d4;
        return to;
    }

    glm::quat Animator::ConvertQuatToGLM(const aiQuaternion &from)
    {
        return glm::quat(from.w, from.x, from.y, from.z);
    }

    glm::vec3 Animator::ConvertVec3ToGLM(const aiVector3D &from)
    {
        return glm::vec3(from.x, from.y, from.z);
    }
}
