#include "SceneSerializer.h"
#include "Scene.h"
#include "Entity.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "LightComponent.h"
#include "AnimationComponent.h"
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

        if (auto *anim = entity->GetComponent<AnimationComponent>())
        {
            j["animationGraph"] = SerializeAnimationGraph(anim);
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

        if (entityJson.contains("animationGraph"))
        {
            DeserializeAnimationGraph(&entity, entityJson["animationGraph"]);
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

    json SceneSerializer::SerializeAnimationGraph(AnimationComponent *comp)
    {
        json j;
        j["enabled"] = comp->graphEnabled;
        j["activeState"] = comp->activeState;
        j["entryNodePosition"] = {comp->entryNodePosition.x, comp->entryNodePosition.y};

        j["library"] = json::array();
        for (const auto &entry : comp->animationLibrary)
        {
            json entryJson;
            entryJson["displayName"] = entry.displayName;
            entryJson["sourceModelPath"] = entry.sourceModelPath;
            entryJson["sourceClipIndex"] = entry.sourceClipIndex;
            entryJson["durationSeconds"] = entry.durationSeconds;
            entryJson["channelBoneNames"] = entry.channelBoneNames;
            entryJson["rootTranslationDelta"] = {entry.rootTranslationDelta.x, entry.rootTranslationDelta.y, entry.rootTranslationDelta.z};
            entryJson["rootRotationDeltaEuler"] = {entry.rootRotationDeltaEuler.x, entry.rootRotationDeltaEuler.y, entry.rootRotationDeltaEuler.z};
            entryJson["rootScaleDelta"] = {entry.rootScaleDelta.x, entry.rootScaleDelta.y, entry.rootScaleDelta.z};
            j["library"].push_back(entryJson);
        }

        j["states"] = json::array();
        for (const auto &state : comp->graphStates)
        {
            json stateJson;
            stateJson["name"] = state.name;
            stateJson["libraryClip"] = state.libraryClip;
            stateJson["modelPath"] = state.modelPath;
            stateJson["clipIndex"] = state.clipIndex;
            stateJson["loop"] = state.loop;
            stateJson["nodePosition"] = {state.nodePosition.x, state.nodePosition.y};
            stateJson["durationSeconds"] = state.durationSeconds;
            stateJson["rootTranslationDelta"] = {state.rootTranslationDelta.x, state.rootTranslationDelta.y, state.rootTranslationDelta.z};
            stateJson["rootRotationDeltaEuler"] = {state.rootRotationDeltaEuler.x, state.rootRotationDeltaEuler.y, state.rootRotationDeltaEuler.z};
            stateJson["rootScaleDelta"] = {state.rootScaleDelta.x, state.rootScaleDelta.y, state.rootScaleDelta.z};
            stateJson["rootMotionEnabled"] = state.rootMotionEnabled;
            stateJson["rootMotionAllowVertical"] = state.rootMotionAllowVertical;
            stateJson["rootMotionScale"] = state.rootMotionScale;
            stateJson["rootMotionMaxSpeed"] = state.rootMotionMaxSpeed;

            stateJson["transformFilters"] = json::array();
            for (const auto &filter : state.transformFilters)
            {
                json filterJson;
                filterJson["boneName"] = filter.boneName;
                filterJson["lockPosX"] = filter.lockPosX;
                filterJson["lockPosY"] = filter.lockPosY;
                filterJson["lockPosZ"] = filter.lockPosZ;
                filterJson["posWeightX"] = filter.posWeightX;
                filterJson["posWeightY"] = filter.posWeightY;
                filterJson["posWeightZ"] = filter.posWeightZ;
                filterJson["lockRotX"] = filter.lockRotX;
                filterJson["lockRotY"] = filter.lockRotY;
                filterJson["lockRotZ"] = filter.lockRotZ;
                filterJson["rotWeightX"] = filter.rotWeightX;
                filterJson["rotWeightY"] = filter.rotWeightY;
                filterJson["rotWeightZ"] = filter.rotWeightZ;
                filterJson["lockScaleX"] = filter.lockScaleX;
                filterJson["lockScaleY"] = filter.lockScaleY;
                filterJson["lockScaleZ"] = filter.lockScaleZ;
                filterJson["scaleWeightX"] = filter.scaleWeightX;
                filterJson["scaleWeightY"] = filter.scaleWeightY;
                filterJson["scaleWeightZ"] = filter.scaleWeightZ;
                stateJson["transformFilters"].push_back(filterJson);
            }
            j["states"].push_back(stateJson);
        }

        j["transitions"] = json::array();
        for (const auto &transition : comp->graphTransitions)
        {
            json transitionJson;
            transitionJson["from"] = transition.fromState;
            transitionJson["to"] = transition.toState;
            transitionJson["trigger"] = transition.trigger;
            transitionJson["hasExitTime"] = transition.hasExitTime;
            transitionJson["exitTimeNormalized"] = transition.exitTimeNormalized;
            transitionJson["blendDuration"] = transition.blendDuration;
            j["transitions"].push_back(transitionJson);
        }

        j["inputs"] = json::array();
        for (const auto &binding : comp->inputBindings)
        {
            json bindingJson;
            bindingJson["trigger"] = binding.trigger;
            bindingJson["key"] = binding.key;
            j["inputs"].push_back(bindingJson);
        }

        return j;
    }

    void SceneSerializer::DeserializeAnimationGraph(Entity *entity, const json &j)
    {
        auto &anim = entity->AddComponent<AnimationComponent>();

        if (j.contains("enabled"))
            anim.graphEnabled = j["enabled"];
        if (j.contains("activeState"))
            anim.activeState = j["activeState"];
        if (j.contains("entryNodePosition") && j["entryNodePosition"].is_array() && j["entryNodePosition"].size() >= 2)
        {
            anim.entryNodePosition = glm::vec2(j["entryNodePosition"][0],
                                               j["entryNodePosition"][1]);
        }

        anim.animationLibrary.clear();
        if (j.contains("library"))
        {
            for (const auto &entryJson : j["library"])
            {
                AnimationComponent::AnimationLibraryEntry entry;
                if (entryJson.contains("displayName"))
                    entry.displayName = entryJson["displayName"].get<std::string>();
                if (entryJson.contains("sourceModelPath"))
                    entry.sourceModelPath = entryJson["sourceModelPath"].get<std::string>();
                if (entryJson.contains("sourceClipIndex"))
                    entry.sourceClipIndex = entryJson["sourceClipIndex"].get<int>();
                if (entryJson.contains("durationSeconds"))
                    entry.durationSeconds = entryJson["durationSeconds"].get<float>();
                if (entryJson.contains("channelBoneNames"))
                    entry.channelBoneNames = entryJson["channelBoneNames"].get<std::vector<std::string>>();
                if (entryJson.contains("rootTranslationDelta") && entryJson["rootTranslationDelta"].is_array() && entryJson["rootTranslationDelta"].size() >= 3)
                {
                    entry.rootTranslationDelta = glm::vec3(entryJson["rootTranslationDelta"][0],
                                                           entryJson["rootTranslationDelta"][1],
                                                           entryJson["rootTranslationDelta"][2]);
                }
                if (entryJson.contains("rootRotationDeltaEuler") && entryJson["rootRotationDeltaEuler"].is_array() && entryJson["rootRotationDeltaEuler"].size() >= 3)
                {
                    entry.rootRotationDeltaEuler = glm::vec3(entryJson["rootRotationDeltaEuler"][0],
                                                             entryJson["rootRotationDeltaEuler"][1],
                                                             entryJson["rootRotationDeltaEuler"][2]);
                }
                if (entryJson.contains("rootScaleDelta") && entryJson["rootScaleDelta"].is_array() && entryJson["rootScaleDelta"].size() >= 3)
                {
                    entry.rootScaleDelta = glm::vec3(entryJson["rootScaleDelta"][0],
                                                     entryJson["rootScaleDelta"][1],
                                                     entryJson["rootScaleDelta"][2]);
                }
                anim.animationLibrary.push_back(entry);
            }
        }

        anim.graphStates.clear();
        if (j.contains("states"))
        {
            for (const auto &stateJson : j["states"])
            {
                AnimationComponent::GraphState state;
                if (stateJson.contains("name"))
                    state.name = stateJson["name"].get<std::string>();
                if (stateJson.contains("libraryClip"))
                    state.libraryClip = stateJson["libraryClip"].get<int>();
                if (stateJson.contains("modelPath"))
                    state.modelPath = stateJson["modelPath"].get<std::string>();
                if (stateJson.contains("clipIndex"))
                    state.clipIndex = stateJson["clipIndex"].get<int>();
                if (stateJson.contains("loop"))
                    state.loop = stateJson["loop"].get<bool>();
                if (stateJson.contains("nodePosition") && stateJson["nodePosition"].is_array() && stateJson["nodePosition"].size() >= 2)
                {
                    state.nodePosition = glm::vec2(stateJson["nodePosition"][0],
                                                   stateJson["nodePosition"][1]);
                }
                if (stateJson.contains("durationSeconds"))
                    state.durationSeconds = stateJson["durationSeconds"].get<float>();
                if (stateJson.contains("rootTranslationDelta") && stateJson["rootTranslationDelta"].is_array() && stateJson["rootTranslationDelta"].size() >= 3)
                {
                    state.rootTranslationDelta = glm::vec3(stateJson["rootTranslationDelta"][0],
                                                           stateJson["rootTranslationDelta"][1],
                                                           stateJson["rootTranslationDelta"][2]);
                }
                if (stateJson.contains("rootRotationDeltaEuler") && stateJson["rootRotationDeltaEuler"].is_array() && stateJson["rootRotationDeltaEuler"].size() >= 3)
                {
                    state.rootRotationDeltaEuler = glm::vec3(stateJson["rootRotationDeltaEuler"][0],
                                                             stateJson["rootRotationDeltaEuler"][1],
                                                             stateJson["rootRotationDeltaEuler"][2]);
                }
                if (stateJson.contains("rootScaleDelta") && stateJson["rootScaleDelta"].is_array() && stateJson["rootScaleDelta"].size() >= 3)
                {
                    state.rootScaleDelta = glm::vec3(stateJson["rootScaleDelta"][0],
                                                     stateJson["rootScaleDelta"][1],
                                                     stateJson["rootScaleDelta"][2]);
                }
                if (stateJson.contains("rootMotionEnabled"))
                    state.rootMotionEnabled = stateJson["rootMotionEnabled"].get<bool>();
                if (stateJson.contains("rootMotionAllowVertical"))
                    state.rootMotionAllowVertical = stateJson["rootMotionAllowVertical"].get<bool>();
                if (stateJson.contains("rootMotionScale"))
                    state.rootMotionScale = stateJson["rootMotionScale"].get<float>();
                if (stateJson.contains("rootMotionMaxSpeed"))
                    state.rootMotionMaxSpeed = stateJson["rootMotionMaxSpeed"].get<float>();

                state.transformFilters.clear();
                if (stateJson.contains("transformFilters"))
                {
                    for (const auto &filterJson : stateJson["transformFilters"])
                    {
                        AnimationComponent::GraphState::TransformFilterRule filter;
                        if (filterJson.contains("boneName"))
                            filter.boneName = filterJson["boneName"].get<std::string>();
                        if (filterJson.contains("lockPosX"))
                            filter.lockPosX = filterJson["lockPosX"].get<bool>();
                        if (filterJson.contains("lockPosY"))
                            filter.lockPosY = filterJson["lockPosY"].get<bool>();
                        if (filterJson.contains("lockPosZ"))
                            filter.lockPosZ = filterJson["lockPosZ"].get<bool>();
                        if (filterJson.contains("posWeightX"))
                            filter.posWeightX = filterJson["posWeightX"].get<float>();
                        if (filterJson.contains("posWeightY"))
                            filter.posWeightY = filterJson["posWeightY"].get<float>();
                        if (filterJson.contains("posWeightZ"))
                            filter.posWeightZ = filterJson["posWeightZ"].get<float>();
                        if (filterJson.contains("lockRotX"))
                            filter.lockRotX = filterJson["lockRotX"].get<bool>();
                        if (filterJson.contains("lockRotY"))
                            filter.lockRotY = filterJson["lockRotY"].get<bool>();
                        if (filterJson.contains("lockRotZ"))
                            filter.lockRotZ = filterJson["lockRotZ"].get<bool>();
                        if (filterJson.contains("rotWeightX"))
                            filter.rotWeightX = filterJson["rotWeightX"].get<float>();
                        if (filterJson.contains("rotWeightY"))
                            filter.rotWeightY = filterJson["rotWeightY"].get<float>();
                        if (filterJson.contains("rotWeightZ"))
                            filter.rotWeightZ = filterJson["rotWeightZ"].get<float>();
                        if (filterJson.contains("lockScaleX"))
                            filter.lockScaleX = filterJson["lockScaleX"].get<bool>();
                        if (filterJson.contains("lockScaleY"))
                            filter.lockScaleY = filterJson["lockScaleY"].get<bool>();
                        if (filterJson.contains("lockScaleZ"))
                            filter.lockScaleZ = filterJson["lockScaleZ"].get<bool>();
                        if (filterJson.contains("scaleWeightX"))
                            filter.scaleWeightX = filterJson["scaleWeightX"].get<float>();
                        if (filterJson.contains("scaleWeightY"))
                            filter.scaleWeightY = filterJson["scaleWeightY"].get<float>();
                        if (filterJson.contains("scaleWeightZ"))
                            filter.scaleWeightZ = filterJson["scaleWeightZ"].get<float>();
                        state.transformFilters.push_back(std::move(filter));
                    }
                }
                anim.graphStates.push_back(state);
            }
        }

        if (anim.animationLibrary.empty())
        {
            for (auto &state : anim.graphStates)
            {
                if (state.modelPath.empty())
                    continue;

                int libraryIndex = -1;
                for (size_t i = 0; i < anim.animationLibrary.size(); ++i)
                {
                    const auto &entry = anim.animationLibrary[i];
                    if (entry.sourceModelPath == state.modelPath && entry.sourceClipIndex == state.clipIndex)
                    {
                        libraryIndex = static_cast<int>(i);
                        break;
                    }
                }

                if (libraryIndex == -1)
                {
                    AnimationComponent::AnimationLibraryEntry entry;
                    entry.displayName = state.name;
                    entry.sourceModelPath = state.modelPath;
                    entry.sourceClipIndex = state.clipIndex;
                    entry.durationSeconds = state.durationSeconds;
                    entry.channelBoneNames.clear();
                    entry.rootTranslationDelta = state.rootTranslationDelta;
                    entry.rootRotationDeltaEuler = state.rootRotationDeltaEuler;
                    entry.rootScaleDelta = state.rootScaleDelta;
                    anim.animationLibrary.push_back(entry);
                    libraryIndex = static_cast<int>(anim.animationLibrary.size()) - 1;
                }

                state.libraryClip = libraryIndex;
            }
        }
        else
        {
            for (auto &state : anim.graphStates)
            {
                if (state.libraryClip >= 0 && state.libraryClip < static_cast<int>(anim.animationLibrary.size()))
                    continue;

                for (size_t i = 0; i < anim.animationLibrary.size(); ++i)
                {
                    const auto &entry = anim.animationLibrary[i];
                    if (entry.sourceModelPath == state.modelPath && entry.sourceClipIndex == state.clipIndex)
                    {
                        state.libraryClip = static_cast<int>(i);
                        break;
                    }
                }
            }
        }

        anim.graphTransitions.clear();
        if (j.contains("transitions"))
        {
            for (const auto &transitionJson : j["transitions"])
            {
                AnimationComponent::GraphTransition transition;
                if (transitionJson.contains("from"))
                    transition.fromState = transitionJson["from"].get<int>();
                if (transitionJson.contains("to"))
                    transition.toState = transitionJson["to"].get<int>();
                if (transitionJson.contains("trigger"))
                    transition.trigger = transitionJson["trigger"].get<std::string>();
                if (transitionJson.contains("hasExitTime"))
                    transition.hasExitTime = transitionJson["hasExitTime"].get<bool>();
                if (transitionJson.contains("exitTimeNormalized"))
                    transition.exitTimeNormalized = transitionJson["exitTimeNormalized"].get<float>();
                if (transitionJson.contains("blendDuration"))
                    transition.blendDuration = transitionJson["blendDuration"].get<float>();
                anim.graphTransitions.push_back(transition);
            }
        }

        anim.inputBindings.clear();
        if (j.contains("inputs"))
        {
            for (const auto &inputJson : j["inputs"])
            {
                AnimationComponent::InputBinding input;
                if (inputJson.contains("trigger"))
                    input.trigger = inputJson["trigger"].get<std::string>();
                if (inputJson.contains("key"))
                    input.key = inputJson["key"].get<int>();
                anim.inputBindings.push_back(input);
            }
        }
    }

    json SceneSerializer::SerializeSceneSettings(Scene *scene)
    {
        json j;

        // Skybox settings
        j["skybox"]["enabled"] = scene->IsSkyboxEnabled();
        j["skybox"]["path"] = scene->GetSkyboxPath();

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
                std::cout << "[SceneSerializer] Skybox settings found. enabled="
                          << scene->IsSkyboxEnabled()
                          << " path='" << skyboxPath << "'"
                          << std::endl;
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
