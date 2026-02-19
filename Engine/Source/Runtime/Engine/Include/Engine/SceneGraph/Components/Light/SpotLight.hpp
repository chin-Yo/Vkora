#pragma once

#include "Engine/SceneGraph/Components/Light.hpp"

class SpotLight : public scene::Light
{
    RTTR_ENABLE(scene::Light)
public:
    SpotLight();
    ~SpotLight() override = default;

    SpotLight(SpotLight&& other) noexcept;

    SpotLight& operator=(SpotLight&& other) noexcept;

    glm::vec3 GetDirection() const;
    // Maximum distance the light can reach, used for point and spot lights, default is 0.0
    float range{5.0f};

    // Inner angle of spotlight cone in radians, default is 0.0
    float inner_cone_angle{0.0f};

    // Outer angle of spotlight cone in radians, default is 0.0
    float outer_cone_angle{0.0f};
};
