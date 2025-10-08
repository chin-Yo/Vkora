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

#include "Framework/Core/Sampler.hpp"

#include "Framework/Core/VulkanDevice.hpp"

namespace vkb
{
    Sampler::Sampler(VulkanDevice& d, const VkSamplerCreateInfo& info) : VulkanResource{VK_NULL_HANDLE, &d}
    {
        VK_CHECK_RESULT(vkCreateSampler(GetDevice().GetHandle(), &info, nullptr, &GetHandle()));
    }

    Sampler::Sampler(Sampler&& other) : VulkanResource{std::move(other)}
    {
    }

    Sampler::~Sampler()
    {
        if (HasDevice())
        {
            vkDestroySampler(GetDevice().GetHandle(), GetHandle(), nullptr);
        }
    }
} // namespace vkb
