#include "Animator.h"
#include "AnimationComponent.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace MaraGl
{
    namespace
    {
        std::string ToLowerAscii(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c)
                           { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        bool MatchesGlobPattern(const std::string &pattern, const std::string &text)
        {
            const std::string loweredPattern = ToLowerAscii(pattern);
            const std::string loweredText = ToLowerAscii(text);

            size_t patternIndex = 0;
            size_t textIndex = 0;
            size_t starIndex = std::string::npos;
            size_t matchIndex = 0;

            while (textIndex < loweredText.size())
            {
                if (patternIndex < loweredPattern.size() &&
                    (loweredPattern[patternIndex] == '?' || loweredPattern[patternIndex] == loweredText[textIndex]))
                {
                    ++patternIndex;
                    ++textIndex;
                }
                else if (patternIndex < loweredPattern.size() && loweredPattern[patternIndex] == '*')
                {
                    starIndex = patternIndex++;
                    matchIndex = textIndex;
                }
                else if (starIndex != std::string::npos)
                {
                    patternIndex = starIndex + 1;
                    textIndex = ++matchIndex;
                }
                else
                {
                    return false;
                }
            }

            while (patternIndex < loweredPattern.size() && loweredPattern[patternIndex] == '*')
                ++patternIndex;

            return patternIndex == loweredPattern.size();
        }

        float ClampWeight(float weight)
        {
            return std::clamp(weight, 0.0f, 1.0f);
        }

        void ExtractTRS(const glm::mat4 &m,
                        glm::vec3 &outTranslation,
                        glm::quat &outRotation,
                        glm::vec3 &outScale)
        {
            outTranslation = glm::vec3(m[3]);

            glm::vec3 col0 = glm::vec3(m[0]);
            glm::vec3 col1 = glm::vec3(m[1]);
            glm::vec3 col2 = glm::vec3(m[2]);

            outScale.x = glm::length(col0);
            outScale.y = glm::length(col1);
            outScale.z = glm::length(col2);

            if (outScale.x > 0.000001f)
                col0 /= outScale.x;
            if (outScale.y > 0.000001f)
                col1 /= outScale.y;
            if (outScale.z > 0.000001f)
                col2 /= outScale.z;

            glm::mat3 rotationMat;
            rotationMat[0] = col0;
            rotationMat[1] = col1;
            rotationMat[2] = col2;
            outRotation = glm::normalize(glm::quat_cast(rotationMat));
        }

        void ApplyStateTransformFilter(AnimationComponent *animComp,
                                       const std::string &nodeName,
                                       const glm::vec3 &bindPosition,
                                       const glm::quat &bindRotation,
                                       const glm::vec3 &bindScale,
                                       glm::vec3 &position,
                                       glm::quat &rotation,
                                       glm::vec3 &scale)
        {
            if (!animComp || !animComp->graphEnabled ||
                animComp->activeState < 0 ||
                animComp->activeState >= static_cast<int>(animComp->graphStates.size()))
            {
                return;
            }

            const auto &state = animComp->graphStates[static_cast<size_t>(animComp->activeState)];
            if (state.previewDisableTransformFilters)
                return;
            if (state.transformFilters.empty())
                return;

            for (const auto &filter : state.transformFilters)
            {
                if (filter.boneName.empty())
                    continue;
                if (!MatchesGlobPattern(filter.boneName, nodeName))
                    continue;

                if (filter.lockPosX)
                    position.x = glm::mix(position.x, bindPosition.x, ClampWeight(filter.posWeightX));
                if (filter.lockPosY)
                    position.y = glm::mix(position.y, bindPosition.y, ClampWeight(filter.posWeightY));
                if (filter.lockPosZ)
                    position.z = glm::mix(position.z, bindPosition.z, ClampWeight(filter.posWeightZ));

                glm::vec3 animEuler = glm::eulerAngles(rotation);
                const glm::vec3 bindEuler = glm::eulerAngles(bindRotation);
                if (filter.lockRotX)
                    animEuler.x = glm::mix(animEuler.x, bindEuler.x, ClampWeight(filter.rotWeightX));
                if (filter.lockRotY)
                    animEuler.y = glm::mix(animEuler.y, bindEuler.y, ClampWeight(filter.rotWeightY));
                if (filter.lockRotZ)
                    animEuler.z = glm::mix(animEuler.z, bindEuler.z, ClampWeight(filter.rotWeightZ));
                rotation = glm::normalize(glm::quat(animEuler));

                if (filter.lockScaleX)
                    scale.x = glm::mix(scale.x, bindScale.x, ClampWeight(filter.scaleWeightX));
                if (filter.lockScaleY)
                    scale.y = glm::mix(scale.y, bindScale.y, ClampWeight(filter.scaleWeightY));
                if (filter.lockScaleZ)
                    scale.z = glm::mix(scale.z, bindScale.z, ClampWeight(filter.scaleWeightZ));
            }
        }
    }

    Animator::LocalTransform Animator::SampleLocalTransform(AnimationComponent *animComp,
                                                            const Animation &animation,
                                                            float animationTime,
                                                            const std::string &nodeName,
                                                            const glm::vec3 &bindPosition,
                                                            const glm::quat &bindRotation,
                                                            const glm::vec3 &bindScale)
    {
        LocalTransform transform;
        transform.position = bindPosition;
        transform.rotation = bindRotation;
        transform.scale = bindScale;

        const BoneAnimation *boneAnim = FindBoneAnimation(animation, nodeName);
        if (!boneAnim)
            return transform;

        transform.position = InterpolatePositionVec3(*boneAnim, animationTime);
        transform.rotation = InterpolateRotationQuat(*boneAnim, animationTime);
        transform.scale = InterpolateScaleVec3(*boneAnim, animationTime);

        const std::string lowerNodeName = ToLowerAscii(nodeName);
        const bool isLikelyRootMotionBone =
            (lowerNodeName.find("hips") != std::string::npos) ||
            (lowerNodeName.find("pelvis") != std::string::npos) ||
            (lowerNodeName.find("root") != std::string::npos);

        if (isLikelyRootMotionBone && !boneAnim->positions.empty())
        {
            const glm::vec3 startPosition = boneAnim->positions.front().position;
            transform.position -= startPosition;

            bool allowVerticalRootPose = false;
            if (animComp && animComp->graphEnabled &&
                animComp->activeState >= 0 &&
                animComp->activeState < static_cast<int>(animComp->graphStates.size()))
            {
                allowVerticalRootPose = animComp->graphStates[static_cast<size_t>(animComp->activeState)].rootMotionAllowVertical;
            }

            if (!allowVerticalRootPose)
                transform.position.y = 0.0f;
        }

        ApplyStateTransformFilter(animComp,
                                  nodeName,
                                  bindPosition,
                                  bindRotation,
                                  bindScale,
                                  transform.position,
                                  transform.rotation,
                                  transform.scale);

        return transform;
    }

    Animator::LocalTransform Animator::BlendLocalTransforms(const LocalTransform &primary,
                                                            const LocalTransform &secondary,
                                                            float blendFactor)
    {
        const float t = std::clamp(blendFactor, 0.0f, 1.0f);
        LocalTransform result;
        result.position = glm::mix(primary.position, secondary.position, t);
        result.rotation = glm::normalize(glm::slerp(primary.rotation, secondary.rotation, t));
        result.scale = glm::mix(primary.scale, secondary.scale, t);
        return result;
    }

    void Animator::UpdateAnimation(AnimationComponent *animComp, float deltaTime)
    {
        if (!animComp || animComp->animations.empty() || !animComp->playing)
            return;

        auto advanceTime = [animComp, deltaTime](int animationIndex, float &time, bool loop)
        {
            if (!animComp->IsValidAnimationIndex(animationIndex))
                return;

            const Animation &animation = animComp->animations[static_cast<size_t>(animationIndex)];
            const float ticksPerSecond = animation.ticksPerSecond > 0.0f ? animation.ticksPerSecond : 1.0f;
            const float duration = std::max(animation.duration, 0.0001f);

            time += ticksPerSecond * deltaTime * animComp->playbackSpeed;
            if (loop)
            {
                time = std::fmod(time, duration);
                if (time < 0.0f)
                    time += duration;
            }
            else if (time >= duration)
            {
                time = duration;
            }
        };

        if (animComp->isBlending && animComp->IsValidAnimationIndex(animComp->nextAnimation))
        {
            advanceTime(animComp->currentAnimation, animComp->currentTime, animComp->looping);
            advanceTime(animComp->nextAnimation, animComp->nextTime, animComp->nextLooping);

            animComp->blendTime += deltaTime * animComp->playbackSpeed;
            if (animComp->blendTime >= animComp->blendDuration)
            {
                animComp->currentAnimation = animComp->nextAnimation;
                animComp->currentTime = animComp->nextTime;
                animComp->looping = animComp->nextLooping;
                animComp->ClearBlendState();

                if (animComp->graphTransitionActive)
                {
                    if (animComp->pendingGraphState >= 0)
                        animComp->activeState = animComp->pendingGraphState;
                    animComp->graphTransitionActive = false;
                    animComp->pendingGraphState = -1;
                }
            }

            return;
        }

        if (animComp->IsValidAnimationIndex(animComp->currentAnimation))
        {
            const Animation &currentAnim = animComp->animations[static_cast<size_t>(animComp->currentAnimation)];
            const float duration = std::max(currentAnim.duration, 0.0001f);
            advanceTime(animComp->currentAnimation, animComp->currentTime, animComp->looping);

            const bool reachedEnd = (!animComp->looping && animComp->currentTime >= duration - 0.0001f);
            if (reachedEnd && animComp->queuedAnimation >= 0 && animComp->IsValidAnimationIndex(animComp->queuedAnimation))
            {
                animComp->nextAnimation = animComp->queuedAnimation;
                animComp->nextLooping = animComp->queuedLooping;
                animComp->nextTime = animComp->queuedRestart ? 0.0f : animComp->currentTime;
                animComp->blendDuration = std::max(0.0001f, animComp->queuedBlendDuration);
                animComp->blendTime = 0.0f;
                animComp->isBlending = true;
                animComp->ClearQueuedAnimation();
                return;
            }

            if (reachedEnd && !animComp->looping && animComp->queuedAnimation < 0)
            {
                animComp->playing = false;
            }
        }

        // Debug output every 60 frames (~1 second)
        static int frameCount = 0;
        if (++frameCount % 60 == 0)
        {
            /*  std::cout << "[Animator] Playing: " << currentAnim.name
                        << " | Time: " << animComp->currentTime << "/" << duration
                        << " | Channels: " << currentAnim.boneAnimations.size() << std::endl; */
        }
    }

    glm::mat4 Animator::GetGlobalInverseTransform(const aiScene *scene)
    {
        if (!scene || !scene->mRootNode)
            return glm::mat4(1.0f);

        glm::mat4 rootTransform = ConvertMatrixToGLM(scene->mRootNode->mTransformation);
        return glm::inverse(rootTransform);
    }

    void Animator::CalculateBoneTransform(AnimationComponent *animComp,
                                          const Animation &animation,
                                          const aiNode *node,
                                          const glm::mat4 &parentTransform,
                                          const glm::mat4 &globalInverseTransform)
    {
        CalculateBoneTransform(animComp, animation, nullptr, node, parentTransform, globalInverseTransform, 0.0f);
    }

    void Animator::CalculateBoneTransform(AnimationComponent *animComp,
                                          const Animation &primaryAnimation,
                                          const Animation *secondaryAnimation,
                                          const aiNode *node,
                                          const glm::mat4 &parentTransform,
                                          const glm::mat4 &globalInverseTransform,
                                          float blendFactor)
    {
        if (!node)
            return;

        std::string nodeName = node->mName.C_Str();
        glm::mat4 nodeTransform = ConvertMatrixToGLM(node->mTransformation);

        glm::vec3 bindScale(1.0f);
        glm::quat bindRotation(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 bindPosition(0.0f);
        ExtractTRS(nodeTransform, bindPosition, bindRotation, bindScale);

        LocalTransform primaryPose = SampleLocalTransform(animComp,
                                                          primaryAnimation,
                                                          animComp ? animComp->currentTime : 0.0f,
                                                          nodeName,
                                                          bindPosition,
                                                          bindRotation,
                                                          bindScale);

        LocalTransform finalPose = primaryPose;
        if (secondaryAnimation && blendFactor > 0.0f)
        {
            LocalTransform secondaryPose = SampleLocalTransform(animComp,
                                                                *secondaryAnimation,
                                                                animComp ? animComp->nextTime : 0.0f,
                                                                nodeName,
                                                                bindPosition,
                                                                bindRotation,
                                                                bindScale);
            finalPose = BlendLocalTransforms(primaryPose, secondaryPose, blendFactor);
        }

        nodeTransform = glm::translate(glm::mat4(1.0f), finalPose.position) *
                        glm::mat4_cast(finalPose.rotation) *
                        glm::scale(glm::mat4(1.0f), finalPose.scale);

        glm::mat4 globalTransform = parentTransform * nodeTransform;

        // If this node is a bone, update its final transform
        auto it = animComp->boneInfoMap.find(nodeName);
        if (it != animComp->boneInfoMap.end())
        {
            int boneIndex = it->second.id;
            if (boneIndex >= 0 && boneIndex < (int)animComp->boneTransforms.size())
            {
                glm::mat4 offset = it->second.offset;
                // Convert from model/node space to mesh space for shader skinning.
                animComp->boneTransforms[boneIndex] = globalInverseTransform * globalTransform * offset;
            }
            else
            {
                /* std::cout << "[Animator] ERROR: Bone '" << nodeName << "' has invalid index "
                           << boneIndex << " (array size: " << animComp->boneTransforms.size() << ")" << std::endl; */
            }
        }

        // Recursively process all children (even those without animation channels)
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            CalculateBoneTransform(animComp, primaryAnimation, secondaryAnimation, node->mChildren[i], globalTransform, globalInverseTransform, blendFactor);
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

    glm::vec3 Animator::InterpolatePositionVec3(const BoneAnimation &boneAnim, float animationTime)
    {
        if (boneAnim.positions.empty())
            return glm::vec3(0.0f);
        if (boneAnim.positions.size() == 1)
            return boneAnim.positions[0].position;

        int p0Index = FindPositionIndex(boneAnim, animationTime);
        int p1Index = p0Index + 1;

        float scaleFactor = GetScaleFactor(boneAnim.positions[p0Index].timeStamp,
                                           boneAnim.positions[p1Index].timeStamp,
                                           animationTime);

        return glm::mix(boneAnim.positions[p0Index].position,
                        boneAnim.positions[p1Index].position,
                        scaleFactor);
    }

    glm::mat4 Animator::InterpolatePosition(const BoneAnimation &boneAnim, float animationTime)
    {
        glm::vec3 finalPosition = InterpolatePositionVec3(boneAnim, animationTime);

        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    glm::mat4 Animator::InterpolateRotation(const BoneAnimation &boneAnim, float animationTime)
    {
        return glm::mat4_cast(InterpolateRotationQuat(boneAnim, animationTime));
    }

    glm::mat4 Animator::InterpolateScale(const BoneAnimation &boneAnim, float animationTime)
    {
        return glm::scale(glm::mat4(1.0f), InterpolateScaleVec3(boneAnim, animationTime));
    }

    glm::quat Animator::InterpolateRotationQuat(const BoneAnimation &boneAnim, float animationTime)
    {
        if (boneAnim.rotations.empty())
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (boneAnim.rotations.size() == 1)
            return glm::normalize(boneAnim.rotations[0].orientation);

        int p0Index = FindRotationIndex(boneAnim, animationTime);
        int p1Index = p0Index + 1;

        float scaleFactor = GetScaleFactor(boneAnim.rotations[p0Index].timeStamp,
                                           boneAnim.rotations[p1Index].timeStamp,
                                           animationTime);

        glm::quat finalRotation = glm::slerp(boneAnim.rotations[p0Index].orientation,
                                             boneAnim.rotations[p1Index].orientation,
                                             scaleFactor);
        return glm::normalize(finalRotation);
    }

    glm::vec3 Animator::InterpolateScaleVec3(const BoneAnimation &boneAnim, float animationTime)
    {
        if (boneAnim.scales.empty())
            return glm::vec3(1.0f);
        if (boneAnim.scales.size() == 1)
            return boneAnim.scales[0].scale;

        int p0Index = FindScaleIndex(boneAnim, animationTime);
        int p1Index = p0Index + 1;

        float scaleFactor = GetScaleFactor(boneAnim.scales[p0Index].timeStamp,
                                           boneAnim.scales[p1Index].timeStamp,
                                           animationTime);

        return glm::mix(boneAnim.scales[p0Index].scale,
                        boneAnim.scales[p1Index].scale,
                        scaleFactor);
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
