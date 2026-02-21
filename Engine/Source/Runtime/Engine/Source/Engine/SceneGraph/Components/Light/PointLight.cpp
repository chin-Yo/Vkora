#include "Engine/SceneGraph/Components/Light/PointLight.hpp"

PointLight::PointLight()
{
}

PointLight::PointLight(PointLight&& other) noexcept: scene::Light(std::move(other)),
                                                     range(other.range)
{
}

PointLight& PointLight::operator=(PointLight&& other) noexcept
{
    if (this == &other)
        return *this;
    scene::Light::operator =(std::move(other));
    range = other.range;
    return *this;
}


RTTR_REGISTRATION
{
    using namespace rttr;
    using namespace scene;

    registration::class_<PointLight>("PointLight")
        .constructor<>()
        .property("range", &PointLight::range);
}
