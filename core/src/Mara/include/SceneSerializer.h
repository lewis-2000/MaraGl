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

    class SceneSerializer
    {
    public:
        static bool SaveScene(Scene *scene, const std::string &filepath);
        static bool LoadScene(Scene *scene, const std::string &filepath);

    private:
        // Entity serialization
        static nlohmann::json SerializeEntity(Entity *entity);
        static void DeserializeEntity(Scene *scene, const nlohmann::json &entityJson);

        // Component serializers
        static nlohmann::json SerializeTransform(TransformComponent *comp);
        static nlohmann::json SerializeMesh(MeshComponent *comp);
        static nlohmann::json SerializeLight(LightComponent *comp);
        static nlohmann::json SerializeName(NameComponent *comp);

        // Component deserializers
        static void DeserializeTransform(Entity *entity, const nlohmann::json &j);
        static void DeserializeMesh(Entity *entity, const nlohmann::json &j);
        static void DeserializeLight(Entity *entity, const nlohmann::json &j);
        static void DeserializeName(Entity *entity, const nlohmann::json &j);

        // Scene settings
        static nlohmann::json SerializeSceneSettings(Scene *scene);
        static void DeserializeSceneSettings(Scene *scene, const nlohmann::json &j);
    };
}
