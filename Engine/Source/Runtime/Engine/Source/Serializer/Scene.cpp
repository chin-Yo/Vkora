#include "Serializer/Scene.hpp"

#include "Engine/SceneGraph/ComponentPool.hpp"
#include "Engine/SceneGraph/Node.hpp"
#include "Reflection/JsonSerializer.hpp"

nlohmann::json Serializer::SerializeScene(const scene::Scene& scene)
{
    nlohmann::json j;
    j["name"] = scene.GetName();

    nlohmann::json rootNodes = nlohmann::json::array();
    for (const auto& node : scene.GetNodes())
    {
        rootNodes.push_back(SerializeNode(*node));
    }
    j["root_nodes"] = rootNodes;

    return j;
}

nlohmann::json Serializer::SerializeNode(scene::Node& node)
{
    const auto& t = node.GetTransform();
    const auto& trans = t.GetTranslation();
    const auto& rot = t.GetRotation(); // glm::quat，(w,x,y,z)
    const auto& scale = t.GetScale();

    nlohmann::json j;
    j["name"] = node.GetName();
    j["transform"] = {
        {"translation", {trans.x, trans.y, trans.z}},
        {"rotation", {rot.w, rot.x, rot.y, rot.z}}, // w first!
        {"scale", {scale.x, scale.y, scale.z}}
    };
    // Serialize components
    auto& compMgr = *node.GetScene()->GetComponentManager();
    nlohmann::json components = nlohmann::json::object();
    for (const auto& handle : node.GetComponentHandles())
    {
        scene::Component* compPtr = compMgr.GetComponentByRT(handle.type, handle.index);
        if (!compPtr) continue;

        rttr::instance obj = compPtr;
        std::string typeName = handle.type.get_name().to_string();

        components[typeName] = SerializeComponent(obj, handle.type);
    }
    j["components"] = components;

    // Serialize children
    nlohmann::json children = nlohmann::json::array();
    for (const auto& child : node.GetChildren())
    {
        children.push_back(SerializeNode(*child));
    }
    j["children"] = children;

    return j;
}

nlohmann::json Serializer::SerializeComponent(rttr::instance obj, rttr::type type)
{
    nlohmann::json j = nlohmann::json::object();

    for (auto prop : type.get_properties())
    {
        if (prop.get_metadata("serialize").to_bool())
        {
            // 可选：只序列化带 metadata("serialize") 的属性
        }
        // 或者：序列化所有 public property
        auto value = prop.get_value(obj);
        if (value.is_valid())
        {
            j[prop.get_name().to_string()] = json_serializer::detail::variant_to_json(value);
        }
    }
    return j;
}
