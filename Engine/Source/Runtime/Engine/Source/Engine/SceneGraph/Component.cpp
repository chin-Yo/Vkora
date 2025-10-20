#include "Engine/SceneGraph/Component.hpp"
#include "Engine/SceneGraph/Node.hpp"

namespace scene
{
    Component::Component(const std::string& name) : name(name)
    {
    }

    const std::string& Component::GetName() const
    {
        return name;
    }

    Node* Component::GetOwner() const
    {
        return owner;
    }
}

RTTR_REGISTRATION
{
    using namespace rttr;
    registration::class_<scene::Component>("NodeComponent")
        .constructor<const std::string&>()
        .property("name", &scene::Component::GetName, &scene::Component::SetName);
}
