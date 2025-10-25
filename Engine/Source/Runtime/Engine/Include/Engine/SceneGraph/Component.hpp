#pragma once

#include <string>
#include <rttr/registration>

namespace scene
{
    class Node;
    using NodeID = uint32_t;
    using ComponentTypeID = rttr::type;

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

        Component(Component&& other) noexcept;

        Component& operator=(Component&& other) noexcept;

        virtual ~Component() = default;

        const std::string& GetName() const;

        void SetName(const std::string& name);

        Node* GetOwner() const;

    protected:
        template <typename T>
        friend class ComponentPool;

        std::string name;

        Node* owner = nullptr;
    };
} // namespace scene
