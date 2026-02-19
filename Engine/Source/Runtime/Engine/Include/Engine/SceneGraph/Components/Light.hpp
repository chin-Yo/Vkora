/* Copyright (c) 2018-2024, Arm Limited and Contributors
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
#include <typeinfo>
#include <vector>

#include "Engine/SceneGraph/Component.hpp"
#include "Framework/Common/VkError.hpp"

#include "Framework/Common/glmCommon.hpp"

#include "Framework/Core/ShaderModule.hpp"
#include "rttr/registration_friend.h"


namespace scene
{
    enum LightType
    {
        Directional = 0,
        Point = 1,
        Spot = 2,
        // Insert new light type here
        Max
    };

    class Light : public Component
    {
        RTTR_REGISTRATION_FRIEND
        RTTR_ENABLE(Component)
    public:
        Light();
        Light(const std::string& name);

        Light(Light&& other) noexcept;
        Light& operator=(Light&& other) noexcept;

        virtual ~Light() = default;

        // Color of the light source, default is white (1,1,1)
        glm::vec3 LightColor{1.0f, 1.0f, 1.0f};

        // Brightness intensity of the light, default is 1.0
        float LightIntensity{1.0f};
    };
}
