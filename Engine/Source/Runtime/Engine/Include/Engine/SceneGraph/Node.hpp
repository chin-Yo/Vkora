/* Copyright (c) 2018-2019, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Engine/SceneGraph/Scene.hpp"
#include "Components/Transform.hpp"

namespace scene
{
    class Scene;

    /// @brief A leaf of the tree structure which can have children and a single parent.
    class Node
    {
    public:
        Node(Scene* scene, std::string name, Node* parent = nullptr);

        virtual ~Node() = default;

        const NodeID GetID() const;

        const std::string& GetName() const;

        Transform& GetTransform()
        {
            return transform;
        }

        void SetParent(Node& parent);

        Node* GetParent() const;

        Node* CreateChild(const std::string& childName);

        void Destroy();

        bool IsMarked() const { return isMarkedForDeletion; }
        const std::vector<ComponentHandle>& GetComponentHandles() const { return componentHandles; }
        // loop ref
        /*template <typename T>
        T* AddComponent()
        {
            // 委托给 ComponentManager
            return scene->GetComponentManager()->AddComponent<T>(this);
        }*/

        /*template <typename T>
        T* GetComponent()
        {
            // 委托给 ComponentManager
            return scene->GetComponentManager()->GetComponent<T>(this->id);
        }*/

        void RegisterComponent(const ComponentHandle& handle);

    private:
        friend class Scene;

        void RemoveChild(Node* child)
        {
            children.erase(
                std::remove_if(children.begin(), children.end(),
                               [child](const auto& p) { return p.get() == child; }),
                children.end()
            );
        }

        Scene* scene; // redundancy

        NodeID id;

        bool isMarkedForDeletion = false;

        std::string name;

        Transform transform;

        Node* parent{nullptr};

        std::vector<std::unique_ptr<Node>> children;

        std::vector<ComponentHandle> componentHandles;
    };
}
