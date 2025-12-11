#include "Engine/SceneGraph/Components/Skybox.hpp"
#include "Engine/Texture/TextureFactory.hpp"

namespace scene
{
    Skybox::Skybox(const std::string& name)
        : Component(name)
    {
    }
}

RTTR_REGISTRATION
{
    using namespace rttr;
    registration::class_<scene::Skybox>("scene::Skybox")
        .constructor<const std::string&>();
}
