#pragma once

#include "Engine/SceneGraph/Components/Light.hpp"

class PointLight : public scene::Light
{
    RTTR_ENABLE(scene::Light)
public:
    PointLight();
    ~PointLight() override = default;

    PointLight(PointLight&& other) noexcept;

    PointLight& operator=(PointLight&& other) noexcept;

    // Maximum distance the light can reach, used for point and spot lights, default is 0.0
    float range{5.0f};
};
