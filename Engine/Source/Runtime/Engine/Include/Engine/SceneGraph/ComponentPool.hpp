#pragma once

#include "Engine/SceneGraph/Node.hpp"

namespace scene
{
    class IComponentPool
    {
    public:
        virtual ~IComponentPool() = default;
        virtual void DestroyComponentForNode(uint32_t nodeID) = 0;
    };

    template <typename T>
    class ComponentPool : public IComponentPool
    {
    public:
        size_t AddComponent(Node* owner)
        {
            size_t newIndex = components.size();
            components.emplace_back();
            components.back().owner = owner;

            NodeID id = owner->GetID();
            ownerNodeMap[id] = newIndex;
            indexToNodeMap[newIndex] = id;

            return newIndex;
        }

        T* GetComponent(NodeID nodeID)
        {
            if (ownerNodeMap.count(nodeID))
            {
                return &components[ownerNodeMap[nodeID]];
            }
            return nullptr;
        }

        void DestroyComponentForNode(NodeID nodeID) override
        {
            if (!ownerNodeMap.count(nodeID)) return;

            size_t indexOfRemoved = ownerNodeMap[nodeID];
            size_t indexOfLast = components.size() - 1;
            NodeID nodeOfLast = indexToNodeMap[indexOfLast];

            components[indexOfRemoved] = std::move(components[indexOfLast]);

            ownerNodeMap[nodeOfLast] = indexOfRemoved;
            indexToNodeMap[indexOfRemoved] = nodeOfLast;

            components.pop_back();

            ownerNodeMap.erase(nodeID);
            indexToNodeMap.erase(indexOfLast);
        }

        std::vector<T>& GetData() { return components; }

    private:
        std::vector<T> components;
        std::unordered_map<uint32_t, size_t> ownerNodeMap; // NodeID -> Index
        std::unordered_map<size_t, uint32_t> indexToNodeMap; // Index -> NodeID (用于Swap)}
    };

    class ComponentManager
    {
    public:
        template <typename T>
        T* AddComponent(Node* owner)
        {
            auto pool = GetOrCreatePool<T>();
            size_t index = pool->AddComponent(owner);
            rttr::type t = rttr::type::get<T>();
            owner->RegisterComponent({t, index});
            return &pool->GetData()[index];
        }

        template <typename T>
        T* GetComponentFormNode(NodeID nodeID)
        {
            auto* pool = GetPool<T>();
            if (pool)
                return pool->GetComponent(nodeID);
            return nullptr;
        }

        template <typename T>
        std::vector<T>& GetComponentsByClass()
        {
            auto* pool = GetOrCreatePool<T>();
            return pool->GetData();
        }

        void DestroyComponentsOfNode(Node* node)
        {
            for (const auto& handle : node->GetComponentHandles())
            {
                if (componentPools.count(handle.type))
                {
                    componentPools[handle.type]->DestroyComponentForNode(node->GetID());
                }
            }
        }

    private:
        template <typename T>
        ComponentPool<T>* GetPool()
        {
            ComponentTypeID typeId = rttr::type::get<T>();
            if (componentPools.count(typeId))
            {
                return static_cast<ComponentPool<T>*>(componentPools[typeId].get());
            }
            return nullptr;
        }

        template <typename T>
        ComponentPool<T>* GetOrCreatePool()
        {
            ComponentTypeID typeId = rttr::type::get<T>();
            if (!componentPools.count(typeId))
            {
                componentPools[typeId] = std::make_unique<ComponentPool<T>>();
            }
            return static_cast<ComponentPool<T>*>(componentPools[typeId].get());
        }

        std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentPool>> componentPools;
    };
}
