#include <rttr/registration>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

static void RegisterVec3()
{
    using namespace rttr;
    rttr::registration::class_<glm::vec2>("glm::vec2")
        .constructor<>()(rttr::policy::ctor::as_object)
        .property("x", &glm::vec2::x)
        .property("y", &glm::vec2::y);

    rttr::registration::class_<glm::vec3>("glm::vec3")
        .constructor<>()(rttr::policy::ctor::as_object)
        .property("x", &glm::vec3::x)
        .property("y", &glm::vec3::y)
        .property("z", &glm::vec3::z);

    rttr::registration::class_<glm::vec4>("glm::vec4")
        .constructor<>()(rttr::policy::ctor::as_object)
        .property("x", &glm::vec4::x)
        .property("y", &glm::vec4::y)
        .property("z", &glm::vec4::z)
        .property("w", &glm::vec4::w);
}

static void RegisterQuat()
{
    using namespace rttr;
    registration::class_<glm::quat>("glm::quat")
        .constructor<>()
        .constructor<float, float, float, float>() // w, x, y, z
        .property("w", &glm::quat::w)
        .property("x", &glm::quat::x)
        .property("y", &glm::quat::y)
        .property("z", &glm::quat::z);
}

static void RegisterMat4()
{
    using namespace rttr;
    registration::class_<glm::mat4>("glm::mat4")
        .constructor<>();
    // Mat4 is more complex and is usually not exposed internally. It is only used for parameter passing.
}

RTTR_REGISTRATION
{
    RegisterVec3();
    RegisterQuat();
    RegisterMat4();
}
