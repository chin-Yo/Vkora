/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include <string>

#include "Engine/SceneGraph/Components/Camera.hpp"
#include "Framework/Common/VkError.hpp"
#include "Framework/Common/glmCommon.hpp"
#include "Framework/Common/VkHelpers.hpp"


namespace scene
{
    class PerspectiveCamera : public Camera
    {
    public:
        PerspectiveCamera();
        PerspectiveCamera(const std::string& name);

        PerspectiveCamera(const PerspectiveCamera&) = delete;
        PerspectiveCamera& operator=(const PerspectiveCamera&) = delete;

        PerspectiveCamera(PerspectiveCamera&& other) noexcept;
        PerspectiveCamera& operator=(PerspectiveCamera&& other) noexcept;

        virtual ~PerspectiveCamera() = default;

        void SetAspectRatio(float aspect_ratio);

        void SetFieldOfView(float fov);

        float GetFarPlane() const;

        void SetFarPlane(float zfar);

        float GetNearPlane() const;

        void SetNearPlane(float znear);

        float GetAspectRatio();

        float GetFieldOfView();

        virtual glm::mat4 GetProjection() override;

    private:
        /**
         * @brief Screen size aspect ratio
         */
        float aspect_ratio{1.0f};

        /**
         * @brief Horizontal field of view in radians
         */
        float fov{glm::radians(60.0f)};

        float far_plane{100.0};

        float near_plane{0.1f};
    };
}
