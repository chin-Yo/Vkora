/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024, Bradley Austin Davis. All rights reserved.
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

#include "Framework/Core/Allocated.hpp"
#include "Logging/Logger.hpp"
#include "Framework/Core/VulkanDevice.hpp"

namespace vkb
{
    VmaAllocator& get_memory_allocator()
    {
        static VmaAllocator memory_allocator = VK_NULL_HANDLE;
        return memory_allocator;
    }

    void init(const VmaAllocatorCreateInfo& create_info)
    {
        auto& allocator = get_memory_allocator();
        if (allocator == VK_NULL_HANDLE)
        {
            VkResult result = vmaCreateAllocator(&create_info, &allocator);
            if (result != VK_SUCCESS)
            {
                throw VulkanException{result, "Cannot create allocator"};
            }
        }
    }

    void InitVma(VulkanDevice& device)
    {
        VmaVulkanFunctions vma_vulkan_func{};
        vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vma_vulkan_func.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocator_info{};
        allocator_info.pVulkanFunctions = &vma_vulkan_func;
        allocator_info.physicalDevice = device.get_gpu().get_handle();
        allocator_info.device = device.GetHandle();
        allocator_info.instance = device.get_gpu().get_instance().get_handle();

        bool can_get_memory_requirements = device.is_extension_supported(
            VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
        bool has_dedicated_allocation = device.is_extension_supported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
        if (can_get_memory_requirements && has_dedicated_allocation && device.is_enabled(
            VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
        {
            allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }

        if (device.is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) && device.is_enabled(
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
        {
            allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        if (device.is_extension_supported(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) && device.is_enabled(
            VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
        {
            allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        }

        if (device.is_extension_supported(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) && device.is_enabled(
            VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
        {
            allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
        }

        if (device.is_extension_supported(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) && device.is_enabled(
            VK_KHR_BIND_MEMORY_2_EXTENSION_NAME))
        {
            allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
        }

        if (device.is_extension_supported(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME) && device.is_enabled(
            VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME))
        {
            allocator_info.flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
        }

        init(allocator_info);
    }

    void shutdown()
    {
        auto& allocator = get_memory_allocator();
        if (allocator != VK_NULL_HANDLE)
        {
            VmaTotalStatistics stats;
            vmaCalculateStatistics(allocator, &stats);
            LOG_INFO("Total device memory leaked: {} bytes.", stats.total.statistics.allocationBytes);
            vmaDestroyAllocator(allocator);
            allocator = VK_NULL_HANDLE;
        }
    }
} // namespace vkb
