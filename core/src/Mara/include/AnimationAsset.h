#pragma once

#include "AnimationComponent.h"
#include <string>

namespace MaraGl
{
    class AnimationAsset
    {
    public:
        static bool Save(const std::string &filepath, const Animation &animation);
        static bool Load(const std::string &filepath, Animation &animationOut);
    };
}