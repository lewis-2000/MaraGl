#include "AnimationAsset.h"

#include <json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;

namespace MaraGl
{
    namespace
    {
        json SerializePositionKey(const PositionKey &key)
        {
            return json{{"time", key.timeStamp}, {"value", {key.position.x, key.position.y, key.position.z}}};
        }

        json SerializeRotationKey(const RotationKey &key)
        {
            return json{{"time", key.timeStamp}, {"value", {key.orientation.x, key.orientation.y, key.orientation.z, key.orientation.w}}};
        }

        json SerializeScaleKey(const ScaleKey &key)
        {
            return json{{"time", key.timeStamp}, {"value", {key.scale.x, key.scale.y, key.scale.z}}};
        }

        bool ReadVec3(const json &value, glm::vec3 &outValue)
        {
            if (!value.is_array() || value.size() < 3)
                return false;

            outValue = glm::vec3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
            return true;
        }

        bool ReadQuat(const json &value, glm::quat &outValue)
        {
            if (!value.is_array() || value.size() < 4)
                return false;

            outValue = glm::quat(value[3].get<float>(), value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
            return true;
        }
    }

    bool AnimationAsset::Save(const std::string &filepath, const Animation &animation)
    {
        try
        {
            json root;
            root["version"] = 1;
            root["name"] = animation.name;
            root["duration"] = animation.duration;
            root["ticksPerSecond"] = animation.ticksPerSecond;
            root["boneAnimations"] = json::array();

            for (const auto &boneAnim : animation.boneAnimations)
            {
                json boneJson;
                boneJson["boneName"] = boneAnim.boneName;
                boneJson["positions"] = json::array();
                boneJson["rotations"] = json::array();
                boneJson["scales"] = json::array();

                for (const auto &key : boneAnim.positions)
                    boneJson["positions"].push_back(SerializePositionKey(key));
                for (const auto &key : boneAnim.rotations)
                    boneJson["rotations"].push_back(SerializeRotationKey(key));
                for (const auto &key : boneAnim.scales)
                    boneJson["scales"].push_back(SerializeScaleKey(key));

                root["boneAnimations"].push_back(boneJson);
            }

            std::filesystem::path filePath(filepath);
            if (filePath.has_parent_path())
                std::filesystem::create_directories(filePath.parent_path());

            std::ofstream file(filepath);
            if (!file.is_open())
            {
                std::cerr << "[AnimationAsset] Failed to open file for writing: " << filepath << std::endl;
                return false;
            }

            file << root.dump(4);
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[AnimationAsset] Save failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool AnimationAsset::Load(const std::string &filepath, Animation &animationOut)
    {
        try
        {
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                std::cerr << "[AnimationAsset] Failed to open file for reading: " << filepath << std::endl;
                return false;
            }

            json root;
            file >> root;

            animationOut = Animation{};
            if (root.contains("name"))
                animationOut.name = root["name"].get<std::string>();
            if (root.contains("duration"))
                animationOut.duration = root["duration"].get<float>();
            if (root.contains("ticksPerSecond"))
                animationOut.ticksPerSecond = root["ticksPerSecond"].get<float>();

            if (root.contains("boneAnimations") && root["boneAnimations"].is_array())
            {
                for (const auto &boneJson : root["boneAnimations"])
                {
                    BoneAnimation boneAnim;
                    if (boneJson.contains("boneName"))
                        boneAnim.boneName = boneJson["boneName"].get<std::string>();

                    if (boneJson.contains("positions") && boneJson["positions"].is_array())
                    {
                        for (const auto &keyJson : boneJson["positions"])
                        {
                            if (!keyJson.contains("time") || !keyJson.contains("value"))
                                continue;

                            PositionKey key;
                            key.timeStamp = keyJson["time"].get<float>();
                            ReadVec3(keyJson["value"], key.position);
                            boneAnim.positions.push_back(key);
                        }
                    }

                    if (boneJson.contains("rotations") && boneJson["rotations"].is_array())
                    {
                        for (const auto &keyJson : boneJson["rotations"])
                        {
                            if (!keyJson.contains("time") || !keyJson.contains("value"))
                                continue;

                            RotationKey key;
                            key.timeStamp = keyJson["time"].get<float>();
                            ReadQuat(keyJson["value"], key.orientation);
                            boneAnim.rotations.push_back(key);
                        }
                    }

                    if (boneJson.contains("scales") && boneJson["scales"].is_array())
                    {
                        for (const auto &keyJson : boneJson["scales"])
                        {
                            if (!keyJson.contains("time") || !keyJson.contains("value"))
                                continue;

                            ScaleKey key;
                            key.timeStamp = keyJson["time"].get<float>();
                            ReadVec3(keyJson["value"], key.scale);
                            boneAnim.scales.push_back(key);
                        }
                    }

                    animationOut.boneAnimations.push_back(std::move(boneAnim));
                }
            }

            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[AnimationAsset] Load failed: " << e.what() << std::endl;
            return false;
        }
    }
}