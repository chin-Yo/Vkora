#pragma once

#include "Engine/SceneGraph/Components/Light.hpp"

class DirectionalLight : public scene::Light
{
    RTTR_ENABLE(scene::Light)
public:
    DirectionalLight();
    ~DirectionalLight() override = default;

    DirectionalLight(DirectionalLight&& other) noexcept;

    DirectionalLight& operator=(DirectionalLight&& other) noexcept;

    glm::vec3 GetDirection() const;
};
