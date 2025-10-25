/* Copyright (c) 2020-2024, Arm Limited and Contributors
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

#include "Engine/SceneGraph/Components/Camera.hpp"
#include "Framework/Common/VkError.hpp"
#include "Framework/Common/glmCommon.hpp"
#include "Framework/Common/VkHelpers.hpp"


namespace scene
{
    class OrthographicCamera : public Camera
    {
    public:
        OrthographicCamera(const std::string& name);

        OrthographicCamera(const std::string& name, float left, float right, float bottom, float top, float near_plane,
                           float far_plane);

        virtual ~OrthographicCamera() = default;

        void SetLeft(float left);

        float GetLeft() const;

        void SetRight(float right);

        float GetRight() const;

        void SetBottom(float bottom);

        float GetBottom() const;

        void SetTop(float top);

        float GetTop() const;

        void SetNearPlane(float near_plane);

        float GetNearPlane() const;

        void SetFarPlane(float far_plane);

        float GetFarPlane() const;

        virtual glm::mat4 GetProjection() override;

    private:
        float left{-1.0f};

        float right{1.0f};

        float bottom{-1.0f};

        float top{1.0f};

        float near_plane{0.0f};

        float far_plane{1.0f};
    };
}
