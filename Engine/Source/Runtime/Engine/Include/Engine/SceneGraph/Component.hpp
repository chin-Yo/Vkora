#pragma once

#include <string>
#include <rttr/registration>

namespace scene
{
    class Node;
    using NodeID = uint32_t;
    using ComponentTypeID = rttr::type;

    // 组件句柄，连接 Node 和 ComponentManager
    struct ComponentHandle
    {
        ComponentTypeID type;
        size_t index; // 组件在对应 Pool 中的索引
    };


    /// @brief A generic class which can be used by nodes. 
    class Component
    {
        RTTR_ENABLE()
    public:
        Component() = default;

        Component(const std::string& name);

        Component(Component&& other) = default;

        virtual ~Component() = default;

        const std::string& GetName() const;

        void SetName(const std::string& name) { this->name = name; }

        Node* GetOwner() const;

    private:
        std::string name;

        Node* owner = nullptr;
    };
} // namespace scene
