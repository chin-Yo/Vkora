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


#include "Engine/SceneGraph/Components/PerspectiveCamera.hpp"

#include <glm/gtc/matrix_transform.hpp>


namespace scene
{
    PerspectiveCamera::PerspectiveCamera(const std::string& name) :
        Camera{name}
    {
    }

    void PerspectiveCamera::SetFieldOfView(float new_fov)
    {
        fov = new_fov;
    }

    float PerspectiveCamera::GetFarPlane() const
    {
        return far_plane;
    }

    void PerspectiveCamera::SetFarPlane(float zfar)
    {
        far_plane = zfar;
    }

    float PerspectiveCamera::GetNearPlane() const
    {
        return near_plane;
    }

    void PerspectiveCamera::SetNearPlane(float znear)
    {
        near_plane = znear;
    }

    void PerspectiveCamera::SetAspectRatio(float new_aspect_ratio)
    {
        aspect_ratio = new_aspect_ratio;
    }

    float PerspectiveCamera::GetFieldOfView()
    {
        return fov;
    }

    float PerspectiveCamera::GetAspectRatio()
    {
        return aspect_ratio;
    }

    glm::mat4 PerspectiveCamera::GetProjection()
    {
        // TODO: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
        return glm::perspective(fov, aspect_ratio, far_plane, near_plane);
    }
}
