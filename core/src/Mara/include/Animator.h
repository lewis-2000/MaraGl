#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/scene.h>
#include <map>
#include <vector>

namespace MaraGl
{
    struct AnimationComponent;
    struct Animation;
    struct BoneAnimation;

    class Animator
    {
    public:
        // Update animation and calculate bone transforms
        static void UpdateAnimation(AnimationComponent *animComp, float deltaTime);

        // Returns inverse of scene root transform used for correct skinning space.
        static glm::mat4 GetGlobalInverseTransform(const aiScene *scene);

        // Calculate bone transformation hierarchy
        static void CalculateBoneTransform(AnimationComponent *animComp,
                                           const Animation &animation,
                                           const aiNode *node,
                                           const glm::mat4 &parentTransform,
                                           const glm::mat4 &globalInverseTransform);

    private:
        // Interpolation helpers
        static int FindPositionIndex(const BoneAnimation &boneAnim, float animationTime);
        static int FindRotationIndex(const BoneAnimation &boneAnim, float animationTime);
        static int FindScaleIndex(const BoneAnimation &boneAnim, float animationTime);

        static glm::vec3 InterpolatePositionVec3(const BoneAnimation &boneAnim, float animationTime);
        static glm::mat4 InterpolatePosition(const BoneAnimation &boneAnim, float animationTime);
        static glm::mat4 InterpolateRotation(const BoneAnimation &boneAnim, float animationTime);
        static glm::mat4 InterpolateScale(const BoneAnimation &boneAnim, float animationTime);

        static float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);

        // Helper to find bone animation channel by name
        static const BoneAnimation *FindBoneAnimation(const Animation &animation, const std::string &nodeName);

        // Conversion helpers
        static glm::mat4 ConvertMatrixToGLM(const aiMatrix4x4 &from);
        static glm::quat ConvertQuatToGLM(const aiQuaternion &from);
        static glm::vec3 ConvertVec3ToGLM(const aiVector3D &from);
    };
}
