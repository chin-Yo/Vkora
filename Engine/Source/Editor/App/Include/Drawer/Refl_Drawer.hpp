#pragma once
#include <imgui.h>

#include "rttr/registration"

namespace ui
{
    class PropertyDrawer
    {
    public:
        static bool DrawObject(rttr::instance obj);
        static bool DrawObject(rttr::instance obj, rttr::type& type);

    private:
        static bool DrawProperty(rttr::instance obj, rttr::property prop);

        static bool DrawEnumProperty(rttr::instance obj, rttr::property prop);
        static bool DrawStructProperty(rttr::instance obj, rttr::property prop);
    };
}
