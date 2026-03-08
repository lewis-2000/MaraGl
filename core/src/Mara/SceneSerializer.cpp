#include "SceneSerializer.h"
#include "Scene.h"
#include "Entity.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "LightComponent.h"
#include "Model.h"
#include <fstream>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;

namespace MaraGl
{
    bool SceneSerializer::SaveScene(Scene *scene, const std::string &filepath)
    {
        if (!scene)
            return false;

        try
        {
            json sceneJson;
            sceneJson["version"] = "1.0";

            // Serialize all entities
            json entitiesArray = json::array();
            for (const auto &entity : scene->GetEntities())
            {
                entitiesArray.push_back(SerializeEntity(entity.get()));
            }
            sceneJson["entities"] = entitiesArray;

            // Serialize scene settings
            sceneJson["settings"] = SerializeSceneSettings(scene);

            // Create directory if it doesn't exist
            std::filesystem::path filePath(filepath);
            if (filePath.has_parent_path())
            {
                std::filesystem::path parentDir = filePath.parent_path();
                if (!std::filesystem::exists(parentDir))
                {
                    std::cout << "Creating directory: " << parentDir << std::endl;
                    std::filesystem::create_directories(parentDir);
                }
            }

            // Write to file
            std::ofstream file(filepath);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file for writing: " << filepath << std::endl;
                std::cerr << "Current working directory: " << std::filesystem::current_path() << std::endl;
                return false;
            }

            file << sceneJson.dump(4); // Pretty print with 4 space indentation
            file.close();

            std::cout << "Scene saved successfully to: " << filepath << std::endl;
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error saving scene: " << e.what() << std::endl;
            return false;
        }
    }

    bool SceneSerializer::LoadScene(Scene *scene, const std::string &filepath)
    {
        if (!scene)
            return false;

        json sceneJson;
        if (!ParseSceneFile(filepath, sceneJson))
            return false;

        bool success = ApplySceneData(scene, sceneJson, true);
        if (success)
            std::cout << "Scene loaded successfully from: " << filepath << std::endl;

        return success;
    }

    bool SceneSerializer::ParseSceneFile(const std::string &filepath, nlohmann::json &sceneJsonOut)
    {
        try
        {
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file for reading: " << filepath << std::endl;
                return false;
            }

            file >> sceneJsonOut;
            file.close();
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error parsing scene file: " << e.what() << std::endl;
            return false;
        }
    }

    bool SceneSerializer::ApplySceneData(Scene *scene, const nlohmann::json &sceneJson, bool loadModels)
    {
        if (!scene)
            return false;

        try
        {
            // Check version
            if (sceneJson.contains("version"))
            {
                std::string version = sceneJson["version"];
                std::cout << "Loading scene format version: " << version << std::endl;
            }

            // Deserialize entities
            if (sceneJson.contains("entities"))
            {
                for (const auto &entityJson : sceneJson["entities"])
                {
                    DeserializeEntity(scene, entityJson, loadModels);
                }
            }

            // Deserialize scene settings
            if (sceneJson.contains("settings"))
            {
                DeserializeSceneSettings(scene, sceneJson["settings"]);
            }

            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error applying scene data: " << e.what() << std::endl;
            return false;
        }
    }

    json SceneSerializer::SerializeEntity(Entity *entity)
    {
        json j;
        j["id"] = entity->GetID();

        // Serialize NameComponent
        if (auto *name = entity->GetComponent<NameComponent>())
        {
            j["name"] = SerializeName(name);
        }

        // Serialize TransformComponent
        if (auto *transform = entity->GetComponent<TransformComponent>())
        {
            j["transform"] = SerializeTransform(transform);
        }

        // Serialize MeshComponent
        if (auto *mesh = entity->GetComponent<MeshComponent>())
        {
            j["mesh"] = SerializeMesh(mesh);
        }

        // Serialize LightComponent
        if (auto *light = entity->GetComponent<LightComponent>())
        {
            j["light"] = SerializeLight(light);
        }

        return j;
    }

    void SceneSerializer::DeserializeEntity(Scene *scene, const json &entityJson, bool loadModels)
    {
        // Create entity with default name first
        std::string entityName = "Entity";
        if (entityJson.contains("name"))
        {
            entityName = entityJson["name"]["value"];
        }

        Entity &entity = scene->CreateEntity(entityName);

        // Deserialize components in order
        if (entityJson.contains("name"))
        {
            DeserializeName(&entity, entityJson["name"]);
        }

        if (entityJson.contains("transform"))
        {
            DeserializeTransform(&entity, entityJson["transform"]);
        }

        if (entityJson.contains("mesh"))
        {
            DeserializeMesh(&entity, entityJson["mesh"], loadModels);
        }

        if (entityJson.contains("light"))
        {
            DeserializeLight(&entity, entityJson["light"]);
        }
    }

    json SceneSerializer::SerializeTransform(TransformComponent *comp)
    {
        json j;
        j["position"] = {comp->Position.x, comp->Position.y, comp->Position.z};
        j["rotation"] = {comp->Rotation.x, comp->Rotation.y, comp->Rotation.z};
        j["scale"] = {comp->Scale.x, comp->Scale.y, comp->Scale.z};
        return j;
    }

    json SceneSerializer::SerializeMesh(MeshComponent *comp)
    {
        json j;
        j["visible"] = comp->Visible;
        j["scale"] = comp->ModelScale;
        j["modelPath"] = comp->ModelPath;
        return j;
    }

    json SceneSerializer::SerializeLight(LightComponent *comp)
    {
        json j;
        j["type"] = static_cast<int>(comp->Type);
        j["color"] = {comp->Color.x, comp->Color.y, comp->Color.z};
        j["intensity"] = comp->Intensity;
        j["range"] = comp->Range;
        j["innerAngle"] = comp->InnerAngle;
        j["outerAngle"] = comp->OuterAngle;
        j["enabled"] = comp->Enabled;
        return j;
    }

    json SceneSerializer::SerializeName(NameComponent *comp)
    {
        json j;
        j["value"] = comp->Name;
        return j;
    }

    void SceneSerializer::DeserializeTransform(Entity *entity, const json &j)
    {
        auto &transform = entity->AddComponent<TransformComponent>();
        if (j.contains("position"))
        {
            transform.Position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
        }
        if (j.contains("rotation"))
        {
            transform.Rotation = glm::vec3(j["rotation"][0], j["rotation"][1], j["rotation"][2]);
        }
        if (j.contains("scale"))
        {
            transform.Scale = glm::vec3(j["scale"][0], j["scale"][1], j["scale"][2]);
        }
    }

    void SceneSerializer::DeserializeMesh(Entity *entity, const json &j, bool loadModel)
    {
        auto &mesh = entity->AddComponent<MeshComponent>();

        if (j.contains("visible"))
        {
            mesh.Visible = j["visible"];
        }
        if (j.contains("scale"))
        {
            mesh.ModelScale = j["scale"];
        }
        if (j.contains("modelPath") && !j["modelPath"].get<std::string>().empty())
        {
            std::string modelPath = j["modelPath"];
            mesh.ModelPath = modelPath;

            if (loadModel)
            {
                // Try to load the model
                try
                {
                    mesh.ModelPtr = std::make_shared<Model>(modelPath);
                    std::cout << "Loaded model: " << modelPath << std::endl;
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Failed to load model '" << modelPath << "': " << e.what() << std::endl;
                }
            }
        }
    }

    void SceneSerializer::DeserializeLight(Entity *entity, const json &j)
    {
        auto &light = entity->AddComponent<LightComponent>();

        if (j.contains("type"))
        {
            light.Type = static_cast<LightType>(j["type"].get<int>());
        }
        if (j.contains("color"))
        {
            light.Color = glm::vec3(j["color"][0], j["color"][1], j["color"][2]);
        }
        if (j.contains("intensity"))
        {
            light.Intensity = j["intensity"];
        }
        if (j.contains("range"))
        {
            light.Range = j["range"];
        }
        if (j.contains("innerAngle"))
        {
            light.InnerAngle = j["innerAngle"];
        }
        if (j.contains("outerAngle"))
        {
            light.OuterAngle = j["outerAngle"];
        }
        if (j.contains("enabled"))
        {
            light.Enabled = j["enabled"];
        }
    }

    void SceneSerializer::DeserializeName(Entity *entity, const json &j)
    {
        auto &name = entity->AddComponent<NameComponent>();
        if (j.contains("value"))
        {
            name.Name = j["value"];
        }
    }

    json SceneSerializer::SerializeSceneSettings(Scene *scene)
    {
        json j;

        // Skybox settings
        j["skybox"]["enabled"] = scene->IsSkyboxEnabled();
        // Note: We don't have a way to get the skybox path currently
        // You might want to add a member to Scene to track this

        // Overcast light settings
        j["overcast"]["enabled"] = scene->IsOvercastEnabled();
        auto overcastColor = scene->GetOvercastColor();
        j["overcast"]["color"] = {overcastColor.x, overcastColor.y, overcastColor.z};
        j["overcast"]["intensity"] = scene->GetOvercastIntensity();

        // Editor camera settings
        const auto &camera = scene->GetCameraSettings();
        j["camera"]["position"] = {camera.Position.x, camera.Position.y, camera.Position.z};
        j["camera"]["yaw"] = camera.Yaw;
        j["camera"]["pitch"] = camera.Pitch;
        j["camera"]["moveSpeed"] = camera.MoveSpeed;
        j["camera"]["mouseSensitivity"] = camera.MouseSensitivity;
        j["camera"]["fov"] = camera.FOV;
        j["camera"]["nearClip"] = camera.NearClip;
        j["camera"]["farClip"] = camera.FarClip;

        return j;
    }

    void SceneSerializer::DeserializeSceneSettings(Scene *scene, const json &j)
    {
        // Skybox settings
        if (j.contains("skybox"))
        {
            if (j["skybox"].contains("enabled"))
            {
                scene->SetSkyboxEnabled(j["skybox"]["enabled"]);
            }
            if (j["skybox"].contains("path"))
            {
                std::string skyboxPath = j["skybox"]["path"];
                if (!skyboxPath.empty())
                {
                    scene->LoadSkybox(skyboxPath);
                }
            }
        }

        // Overcast light settings
        if (j.contains("overcast"))
        {
            if (j["overcast"].contains("enabled"))
            {
                scene->SetOvercastEnabled(j["overcast"]["enabled"]);
            }
            if (j["overcast"].contains("color"))
            {
                glm::vec3 color(j["overcast"]["color"][0],
                                j["overcast"]["color"][1],
                                j["overcast"]["color"][2]);
                scene->SetOvercastColor(color);
            }
            if (j["overcast"].contains("intensity"))
            {
                scene->SetOvercastIntensity(j["overcast"]["intensity"]);
            }
        }

        if (j.contains("camera"))
        {
            auto camera = scene->GetCameraSettings();

            if (j["camera"].contains("position"))
            {
                camera.Position = glm::vec3(
                    j["camera"]["position"][0],
                    j["camera"]["position"][1],
                    j["camera"]["position"][2]);
            }
            if (j["camera"].contains("yaw"))
                camera.Yaw = j["camera"]["yaw"];
            if (j["camera"].contains("pitch"))
                camera.Pitch = j["camera"]["pitch"];
            if (j["camera"].contains("moveSpeed"))
                camera.MoveSpeed = j["camera"]["moveSpeed"];
            if (j["camera"].contains("mouseSensitivity"))
                camera.MouseSensitivity = j["camera"]["mouseSensitivity"];
            if (j["camera"].contains("fov"))
                camera.FOV = j["camera"]["fov"];
            if (j["camera"].contains("nearClip"))
                camera.NearClip = j["camera"]["nearClip"];
            if (j["camera"].contains("farClip"))
                camera.FarClip = j["camera"]["farClip"];

            scene->SetCameraSettings(camera);
        }
    }
}
