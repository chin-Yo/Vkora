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


#include "Engine/SceneGraph/Node.hpp"

#include "Engine/SceneGraph/Component.hpp"
#include "Engine/SceneGraph/Scene.hpp"

namespace scene
{
    Node::Node(Scene* scene, std::string name, Node* parent)
        : scene(scene), name(std::move(name)), parent(parent), transform(*this)
    {
        static NodeID nextID = 0;
        id = nextID++;
    }

    const NodeID Node::GetID() const
    {
        return id;
    }

    const std::string& Node::GetName() const
    {
        return name;
    }

    void Node::SetName(const std::string& name)
    {
        this->name = name;
    }

    void Node::SetParent(Node& p)
    {
        parent = &p;

        transform.InvalidateWorldMatrix();
    }

    Node* Node::GetParent() const
    {
        return parent;
    }

    Node* Node::CreateChild(const std::string& childName)
    {
        auto child = std::make_unique<Node>(scene, childName, this);
        Node* ptr = child.get();
        children.push_back(std::move(child));
        return ptr;
    }

    const std::vector<std::unique_ptr<Node>>& Node::GetChildren() const
    {
        return children;
    }

    void Node::Destroy()
    {
        isMarkedForDeletion = true;
        scene->MarkNodeForDeletion(this);
        // 递归标记所有子节点
        for (auto& child : children)
        {
            child->Destroy();
        }
    }

    void Node::RegisterComponent(const ComponentHandle& handle)
    {
        componentHandles.push_back(handle);
    }
}
