#include "Engine/SceneGraph/Components/Light/SpotLight.hpp"

#include "Core/Math/MathUtils.h"
#include "Engine/SceneGraph/Node.hpp"

SpotLight::SpotLight()
{
}

SpotLight::SpotLight(SpotLight&& other) noexcept:
    scene::Light(std::move(other)),
    range(other.range),
    inner_cone_angle(other.inner_cone_angle),
    outer_cone_angle(other.outer_cone_angle)
{
}

SpotLight& SpotLight::operator=(SpotLight&& other) noexcept
{
    if (this == &other)
        return *this;
    scene::Light::operator =(std::move(other));
    range = other.range;
    inner_cone_angle = other.inner_cone_angle;
    outer_cone_angle = other.outer_cone_angle;
    return *this;
}

glm::vec3 SpotLight::GetDirection() const
{
    return MathUtils::EulerToDirection(GetOwner()->GetTransform().GetRotationEuler());
}

RTTR_REGISTRATION
{
    using namespace rttr;
    using namespace scene;

    registration::class_<SpotLight>("SpotLight")
        .constructor<>()
        .property("range", &SpotLight::range)
        .property("inner_cone_angle", &SpotLight::inner_cone_angle)
        .property("outer_cone_angle", &SpotLight::outer_cone_angle);
}
