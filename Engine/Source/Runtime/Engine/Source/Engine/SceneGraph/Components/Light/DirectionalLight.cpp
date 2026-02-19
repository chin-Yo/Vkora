#include "Engine/SceneGraph/Components/Light/DirectionalLight.hpp"

#include "Core/Math/MathUtils.h"
#include "Engine/SceneGraph/Node.hpp"

DirectionalLight::DirectionalLight()
{
}

DirectionalLight::DirectionalLight(DirectionalLight&& other) noexcept
    : scene::Light(std::move(other))
{
}

DirectionalLight& DirectionalLight::operator=(DirectionalLight&& other) noexcept
{
    if (this == &other)
        return *this;
    scene::Light::operator =(std::move(other));
    return *this;
}


glm::vec3 DirectionalLight::GetDirection() const
{
    return MathUtils::EulerToDirection(GetOwner()->GetTransform().GetRotationEuler());
}


RTTR_REGISTRATION
{
    using namespace rttr;
    using namespace scene;

    registration::class_<DirectionalLight>("DirectionalLight")
        .constructor<>();
}
