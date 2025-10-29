#include "Engine/SceneGraph/Component.hpp"
#include "Engine/SceneGraph/Node.hpp"

namespace scene
{
    Component::Component(const std::string& name) : name(name)
    {
    }

    Component& Component::operator=(Component&& other) noexcept
    {
        if (this != &other)
        {
            name = std::move(other.name);
            owner = other.owner;
        }
        return *this;
    }

    Component::Component(Component&& other) noexcept
    {
        name = std::move(other.name);
        owner = other.owner;
    }

    const std::string& Component::GetName() const
    {
        return name;
    }

    void Component::SetName(const std::string& name)
    {
        this->name = name;
    }

    Node* Component::GetOwner() const
    {
        return owner;
    }
}

RTTR_REGISTRATION
{
    using namespace rttr;
    registration::class_<scene::Component>("scene::Component")
        .constructor<const std::string&>()
        .property("name", &scene::Component::GetName, &scene::Component::SetName);
}
