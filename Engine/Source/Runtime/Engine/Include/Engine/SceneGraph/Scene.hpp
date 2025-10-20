#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "rttr/type.h"


namespace scene
{
    class ComponentManager;
    class Node;
    class Component;
    class SubMesh;

    /// @brief A collection of nodes organized in a tree structure.
    ///		   It can contain more than one root node.
    class Scene
    {
    public:
        Scene() = default;

        Scene(const std::string& name);

        void SetName(const std::string& name);

        const std::string& GetName() const;

        void SetNodes(std::vector<std::unique_ptr<Node>>&& nodes);

        void AddNode(std::unique_ptr<Node> node);

        void MarkNodeForDeletion(Node* node);

        ComponentManager* GetComponentManager() const;

    private:
        std::string name;

        /// List of all the nodes
        std::vector<std::unique_ptr<Node>> nodes;
        /// 
        std::unique_ptr<ComponentManager> componentManager;

        std::vector<Node*> nodesToDestroy;
    };
} // namespace sg
