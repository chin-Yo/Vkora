/* Copyright (c) 2018-2023, Arm Limited and Contributors
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

#include "Engine/SceneGraph/Scene.hpp"
#include <queue>
#include "Engine/SceneGraph/Node.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"

namespace scene
{
    Scene::Scene(const std::string& name) :
        name{name}
    {
    }

    void Scene::SetName(const std::string& new_name)
    {
        name = new_name;
    }

    const std::string& Scene::GetName() const
    {
        return name;
    }

    void Scene::SetNodes(std::vector<std::unique_ptr<Node>>&& n)
    {
        assert(nodes.empty() && "Scene nodes were already set");
        nodes = std::move(n);
    }

    void Scene::AddNode(std::unique_ptr<Node> n)
    {
        nodes.emplace_back(std::move(n));
    }

    void Scene::MarkNodeForDeletion(Node* node)
    {
        if (std::find(nodesToDestroy.begin(), nodesToDestroy.end(), node) == nodesToDestroy.end())
        {
            nodesToDestroy.push_back(node);
        }
    }

    ComponentManager* Scene::GetComponentManager() const
    {
        return componentManager.get();
    }
}
