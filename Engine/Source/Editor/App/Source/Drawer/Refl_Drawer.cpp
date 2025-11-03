#include "Drawer/Refl_Drawer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

bool ui::PropertyDrawer::DrawObject(rttr::instance obj)
{
    if (!obj.is_valid()) return false;

    bool modified = false;
    rttr::type type = obj.get_type();
    for (auto& prop : type.get_properties())
    {
        if (DrawProperty(obj, prop))
        {
            modified = true;
        }
    }
    return modified;
}

bool ui::PropertyDrawer::DrawProperty(rttr::instance obj, rttr::property prop)
{
    bool is_readonly = prop.is_readonly();
    if (is_readonly)
    {
        ImGui::BeginDisabled();
    }

    bool modified = false;
    std::string label = prop.get_name().to_string();
    rttr::type prop_type = prop.get_type();
    rttr::variant value = prop.get_value(obj);

    if (prop_type.is_enumeration())
    {
        modified = DrawEnumProperty(obj, prop);
    }
    else if (prop_type == rttr::type::get<bool>())
    {
        bool v = value.get_value<bool>();
        if (ImGui::Checkbox(label.c_str(), &v))
        {
            prop.set_value(obj, v);
            modified = true;
        }
    }
    else if (prop_type == rttr::type::get<float>())
    {
        float v = value.get_value<float>();
        if (ImGui::DragFloat(label.c_str(), &v, 0.1f))
        {
            prop.set_value(obj, v);
            modified = true;
        }
    }
    else if (prop_type == rttr::type::get<glm::vec3>())
    {
        glm::vec3 v = value.get_value<glm::vec3>();
        bool changed = false;
        if (prop.get_metadata("widget") == "color")
        {
            changed = ImGui::ColorEdit3(label.c_str(), glm::value_ptr(v));
        }
        else
        {
            changed = ImGui::DragFloat3(label.c_str(), glm::value_ptr(v), 0.1f);
        }
        if (changed)
        {
            prop.set_value(obj, v);
            modified = true;
        }
    }
    else if (prop_type.is_class() && prop_type.get_properties().size() > 0)
    {
        modified = DrawStructProperty(obj, prop);
    }
    else
    {
        ImGui::Text("%s: (Unsupported Type: %s)", label.c_str(), prop_type.get_name().to_string().c_str());
    }

    if (is_readonly)
    {
        ImGui::EndDisabled();
    }
    return modified;
}

bool ui::PropertyDrawer::DrawEnumProperty(rttr::instance obj, rttr::property prop)
{
    rttr::type prop_type = prop.get_type();
    if (!prop_type.is_enumeration()) return false;

    auto enumeration = prop_type.get_enumeration();

    auto names = enumeration.get_names();
    auto values = enumeration.get_values();

    std::vector<std::string> name_strings;
    std::vector<const char*> c_names;
    std::vector<rttr::variant> value_list; // 存储值列表

    name_strings.reserve(names.size());
    c_names.reserve(names.size());

    for (const auto& name : names)
    {
        name_strings.push_back(name.to_string());
        c_names.push_back(name_strings.back().c_str());
    }

    for (const auto& value : values)
    {
        value_list.push_back(value);
    }

    int current_item_index = -1;
    rttr::variant current_enum_value = prop.get_value(obj);
    if (current_enum_value.is_valid())
    {
        std::string current_str = current_enum_value.to_string();
        for (int i = 0; i < static_cast<int>(name_strings.size()); ++i)
        {
            if (name_strings[i] == current_str)
            {
                current_item_index = i;
                break;
            }
        }
    }

    std::string label = prop.get_name().to_string();
    if (ImGui::Combo(label.c_str(), &current_item_index,
                     c_names.data(), static_cast<int>(c_names.size())))
    {
        if (current_item_index >= 0 && current_item_index < static_cast<int>(value_list.size()))
        {
            prop.set_value(obj, value_list[current_item_index]);
            return true;
        }
    }
    return false;
}

bool ui::PropertyDrawer::DrawStructProperty(rttr::instance obj, rttr::property prop)
{
    bool modified = false;
    std::string label = prop.get_name().to_string();

    if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 核心技巧: 
        // 1. 获取结构体的一个副本。
        rttr::variant struct_variant = prop.get_value(obj);

        // 2. 在这个副本上递归调用 DrawObject。
        //    我们传递的是 struct_variant 的实例，它是一个值，而不是指针。
        if (DrawObject(struct_variant))
        {
            // 3. 如果副本被修改了，通过 setter 将整个修改后的结构体设置回去。
            prop.set_value(obj, struct_variant);
            modified = true;
        }
        ImGui::TreePop();
    }
    return modified;
}
