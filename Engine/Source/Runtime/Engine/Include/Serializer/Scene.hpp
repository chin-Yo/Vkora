#pragma once
#include <nlohmann/json.hpp>

#include "Engine/SceneGraph/Scene.hpp"

namespace scene
{
    class Transform;
}

class Serializer
{
public:
    static nlohmann::json SerializeScene(const scene::Scene& scene);

    static nlohmann::json SerializeNode(scene::Node& node);

    static nlohmann::json SerializeComponent(rttr::instance obj, rttr::type type);
};
