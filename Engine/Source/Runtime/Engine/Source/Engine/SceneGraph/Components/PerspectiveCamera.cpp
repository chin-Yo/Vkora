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

#include "Framework/Rendering/Subpass.hpp"


namespace scene
{
    PerspectiveCamera::PerspectiveCamera()
        : Camera{"Perspective Camera"}
    {
    }

    PerspectiveCamera::PerspectiveCamera(const std::string& name) :
        Camera{name}
    {
    }

    PerspectiveCamera::PerspectiveCamera(PerspectiveCamera&& other) noexcept
        : Camera{other.GetName()}
    {
        aspect_ratio = other.aspect_ratio;
        fov = other.fov;
        far_plane = other.far_plane;
        near_plane = other.near_plane;
    }

    PerspectiveCamera& PerspectiveCamera::operator=(PerspectiveCamera&& other) noexcept
    {
        aspect_ratio = other.aspect_ratio;
        fov = other.fov;
        far_plane = other.far_plane;
        near_plane = other.near_plane;
        return *this;
    }

    void PerspectiveCamera::SetFieldOfView(float new_fov)
    {
        fov = glm::radians(new_fov);
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

    // TODO: Learn
    const std::vector<Cascade>& PerspectiveCamera::GetCascades(uint32_t cascade_num, const glm::vec3& lightDir,
                                                               float cascadeSplitLambda)
    {
        glm::vec3 lightDirNor = glm::normalize(lightDir);
        Cascades.resize(cascade_num);

        float nearClip = GetNearPlane();
        float farClip = GetFarPlane();
        float clipRange = farClip - nearClip;

        // Since reversed depth is used,
        // but split calculation is still based on geometric distance, minZ = near, maxZ = far
        float minZ = nearClip;
        float maxZ = farClip;
        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        // Calculate cascade split depths (normalized to [0,1])
        std::vector<float> cascadeSplits(cascade_num);
        for (uint32_t i = 0; i < cascade_num; ++i)
        {
            float p = (i + 1) / static_cast<float>(cascade_num);
            float logSplit = minZ * std::pow(ratio, p);
            float uniformSplit = minZ + range * p;
            float d = cascadeSplitLambda * (logSplit - uniformSplit) + uniformSplit;
            cascadeSplits[i] = (d - nearClip) / clipRange; // Normalized to [0,1]
        }

        // Construct the 8 corners of the full frustum in world space
        glm::vec3 frustumCorners[8] = {
            // [0]-[3]: Near Plane (Note: Z is now 1.0f)
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),

            // [4]-[7]: Far Plane (Note: Z is now 0.0f)
            glm::vec3(-1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f, -1.0f, 0.0f)
        };

        // Camera's View-Projection matrix (Note: GetProjection already uses reversed depth)
        glm::mat4 viewProj = GetPreRotation() * vkb::vulkan_style_projection(
            GetProjection()) * GetViewMatrix();
        glm::mat4 invViewProj = glm::inverse(viewProj);

        // Transform NDC corners to world space
        for (uint32_t i = 0; i < 8; ++i)
        {
            glm::vec4 ndc = glm::vec4(frustumCorners[i], 1.0f);
            glm::vec4 world = invViewProj * ndc;
            frustumCorners[i] = glm::vec3(world / world.w);
        }

        // Build the light View-Projection matrix for each cascade
        float lastSplit = 0.0f;
        for (uint32_t i = 0; i < cascade_num; ++i)
        {
            float split = cascadeSplits[i];

            // Extract the sub-frustum for the current cascade (8 points)
            glm::vec3 cascadeCorners[8];
            for (uint32_t j = 0; j < 4; ++j)
            {
                // Near plane point: Interpolate from the original near along the ray to get the lastSplit depth
                glm::vec3 rayNear = frustumCorners[j + 4] - frustumCorners[j];
                cascadeCorners[j] = frustumCorners[j] + rayNear * lastSplit;

                // Far plane point: Interpolate to get the split depth
                cascadeCorners[j + 4] = frustumCorners[j] + rayNear * split;
            }

            // Calculate the bounding sphere center
            glm::vec3 center(0.0f);
            for (const auto& corner : cascadeCorners)
            {
                center += corner;
            }
            center /= 8.0f;

            // Calculate the bounding sphere radius
            float radius = 0.0f;
            for (const auto& corner : cascadeCorners)
            {
                radius = std::max(radius, glm::distance(center, corner));
            }
            // Texel snapping: Align to 1/16 units to reduce shadow swimming
            radius = std::ceil(radius * 16.0f) / 16.0f;
            // Build the light view matrix
            // Note: lightDir is the light direction (e.g., sun direction), so the light "position" is at center - lightDir * radius
            glm::vec3 lightPos = center - lightDirNor * radius;
            glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0.0f, 1.0f, 0.0f));

            // Build the orthographic projection (covering [-radius, radius] in x/y, [0, 2*radius] in z)
            glm::mat4 lightProj = glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, radius * 2.0f);
            vkb::vulkan_style_projection(lightProj);
            // Store the result
            Cascades[i].viewProjMatrix = lightProj * lightView;

            // splitDepth is used in the main rendering to determine the cascade index
            // Note: This stores the depth along the camera's view direction in world space (positive value)
            Cascades[i].splitDepth = (nearClip + split * clipRange) * -1.0f;

            lastSplit = split;
        }

        return Cascades;
    }

    const std::vector<Cascade>& PerspectiveCamera::GetCascades() const
    {
        return Cascades;
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
