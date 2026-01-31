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

    // TODO: Learn
    std::vector<Cascade> PerspectiveCamera::GetCascades(uint32_t cascade_num, const glm::vec3& lightDir,
                                                        float cascadeSplitLambda)
    {
        std::vector<Cascade> cascades(cascade_num);

        // 1. 获取相机参数
        float nearClip = GetNearPlane(); // 通常为正值，如 0.1f
        float farClip = GetFarPlane(); // 如 100.0f
        float clipRange = farClip - nearClip;

        // 注意：由于使用 reversed depth，但 split 计算仍基于几何距离，所以 minZ = near, maxZ = far
        float minZ = nearClip;
        float maxZ = farClip;
        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        // 2. 计算 cascade 分割深度（归一化到 [0,1]）
        std::vector<float> cascadeSplits(cascade_num);
        for (uint32_t i = 0; i < cascade_num; ++i)
        {
            float p = (i + 1) / static_cast<float>(cascade_num);
            float logSplit = minZ * std::pow(ratio, p);
            float uniformSplit = minZ + range * p;
            float d = cascadeSplitLambda * (logSplit - uniformSplit) + uniformSplit;
            cascadeSplits[i] = (d - nearClip) / clipRange; // 归一化到 [0,1]
        }

        // 3. 构建完整视锥体在世界空间中的 8 个角点
        glm::vec3 frustumCorners[8] = {
            glm::vec3(-1.0f, 1.0f, 0.0f), // Near: TL
            glm::vec3(1.0f, 1.0f, 0.0f), // Near: TR
            glm::vec3(1.0f, -1.0f, 0.0f), // Near: BR
            glm::vec3(-1.0f, -1.0f, 0.0f), // Near: BL
            glm::vec3(-1.0f, 1.0f, 1.0f), // Far: TL
            glm::vec3(1.0f, 1.0f, 1.0f), // Far: TR
            glm::vec3(1.0f, -1.0f, 1.0f), // Far: BR
            glm::vec3(-1.0f, -1.0f, 1.0f) // Far: BL
        };

        // 相机的 View-Projection 矩阵（注意：GetProjection 已使用 reversed depth）
        glm::mat4 view = GetView();
        glm::mat4 proj = GetProjection();
        glm::mat4 viewProj = proj * view;
        glm::mat4 invViewProj = glm::inverse(viewProj);

        // 将 NDC 角点变换到世界空间
        for (uint32_t i = 0; i < 8; ++i)
        {
            glm::vec4 ndc = glm::vec4(frustumCorners[i], 1.0f);
            glm::vec4 world = invViewProj * ndc;
            frustumCorners[i] = glm::vec3(world / world.w);
        }

        // 4. 为每个 cascade 构建光源 View-Projection 矩阵
        float lastSplit = 0.0f;
        for (uint32_t i = 0; i < cascade_num; ++i)
        {
            float split = cascadeSplits[i];

            // 截取当前 cascade 的子视锥体（8 个点）
            glm::vec3 cascadeCorners[8];
            for (uint32_t j = 0; j < 4; ++j)
            {
                // 近平面点：从原 near 沿射线插值得到 lastSplit 深度
                glm::vec3 rayNear = frustumCorners[j + 4] - frustumCorners[j];
                cascadeCorners[j] = frustumCorners[j] + rayNear * lastSplit;

                // 远平面点：插值得到 split 深度
                cascadeCorners[j + 4] = frustumCorners[j] + rayNear * split;
            }

            // 计算包围球中心
            glm::vec3 center(0.0f);
            for (const auto& corner : cascadeCorners)
            {
                center += corner;
            }
            center /= 8.0f;

            // 计算包围球半径
            float radius = 0.0f;
            for (const auto& corner : cascadeCorners)
            {
                radius = std::max(radius, glm::distance(center, corner));
            }

            // Texel snapping：对齐到 1/16 单位，减少 shadow swimming
            radius = std::ceil(radius * 16.0f) / 16.0f;

            // 构建光源视角的 View 矩阵
            // 注意：lightDir 是光照方向（如太阳方向），所以光源“位置”在 center - lightDir * radius
            glm::vec3 lightPos = center - lightDir * radius;
            glm::mat4 lightView = glm::lookAt(lightPos, center, glm::vec3(0.0f, 1.0f, 0.0f));

            // 构建正交投影（覆盖 [-radius, radius] in x/y, [0, 2*radius] in z）
            glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, 0.0f, radius * 2.0f);

            // 存储结果
            cascades[i].viewProjMatrix = lightProj * lightView;

            // splitDepth 用于主渲染中判断 cascade 索引
            // 注意：这里存储的是世界空间中沿相机视线的深度（正值）
            cascades[i].splitDepth = nearClip + split * clipRange;

            lastSplit = split;
        }

        return cascades;
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
