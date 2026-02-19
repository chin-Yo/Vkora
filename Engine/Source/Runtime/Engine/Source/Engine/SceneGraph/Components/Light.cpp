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


#include "Engine/SceneGraph/Components/Light.hpp"


namespace scene
{
    Light::Light()
        : Component{"Light"}
    {
    }

    Light::Light(const std::string& name) :
        Component{name}
    {
    }

    Light::Light(Light&& other) noexcept
        : Component(std::move(other))
    {
        LightColor = other.LightColor;
        LightIntensity = other.LightIntensity;
    }

    Light& Light::operator=(Light&& other) noexcept
    {
        LightColor = other.LightColor;
        LightIntensity = other.LightIntensity;
        return *this;
    }
}

RTTR_REGISTRATION
{
    using namespace rttr;
    using namespace scene;

    registration::enumeration<LightType>("LightType")
    (
        value("Directional", LightType::Directional),
        value("Point", LightType::Point),
        value("Spot", LightType::Spot)
    );
    registration::class_<Light>("Light")
        .constructor<>()
        .constructor<const std::string&>()
        .property("LightColor", &Light::LightColor)
        (
            //using the metadata, inform the UI renderer that this should be a color selector.
            metadata("widget", "color")
        )
        .property("LightIntensity", &Light::LightIntensity);
}
