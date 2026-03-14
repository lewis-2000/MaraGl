#pragma once
#include <string>
#include <json.hpp>

namespace MaraGl
{
    class Scene;
    class Entity;
    struct Component;
    struct TransformComponent;
    struct MeshComponent;
    struct LightComponent;
    struct NameComponent;
    struct AnimationComponent;

    class SceneSerializer
    {
    public:
        static bool SaveScene(Scene *scene, const std::string &filepath);
        static bool LoadScene(Scene *scene, const std::string &filepath);
        static bool ParseSceneFile(const std::string &filepath, nlohmann::json &sceneJsonOut);
        static bool ApplySceneData(Scene *scene, const nlohmann::json &sceneJson, bool loadModels = true);

    private:
        // Entity serialization
        static nlohmann::json SerializeEntity(Entity *entity);
        static void DeserializeEntity(Scene *scene, const nlohmann::json &entityJson, bool loadModels = true);

        // Component serializers
        static nlohmann::json SerializeTransform(TransformComponent *comp);
        static nlohmann::json SerializeMesh(MeshComponent *comp);
        static nlohmann::json SerializeLight(LightComponent *comp);
        static nlohmann::json SerializeName(NameComponent *comp);
        static nlohmann::json SerializeAnimationGraph(AnimationComponent *comp);

        // Component deserializers
        static void DeserializeTransform(Entity *entity, const nlohmann::json &j);
        static void DeserializeMesh(Entity *entity, const nlohmann::json &j, bool loadModel = true);
        static void DeserializeLight(Entity *entity, const nlohmann::json &j);
        static void DeserializeName(Entity *entity, const nlohmann::json &j);
        static void DeserializeAnimationGraph(Entity *entity, const nlohmann::json &j);

        // Scene settings
        static nlohmann::json SerializeSceneSettings(Scene *scene);
        static void DeserializeSceneSettings(Scene *scene, const nlohmann::json &j);
    };
}
