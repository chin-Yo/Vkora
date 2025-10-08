/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include <volk.h>

namespace vkb
{
	class RenderPass;
	class RenderTarget;
	class VulkanDevice;
	
	class Framebuffer
	{
	public:
		Framebuffer(VulkanDevice &device, const RenderTarget &render_target, const RenderPass &render_pass);

		Framebuffer(const Framebuffer &) = delete;

		Framebuffer(Framebuffer &&other);

		~Framebuffer();

		Framebuffer &operator=(const Framebuffer &) = delete;

		Framebuffer &operator=(Framebuffer &&) = delete;

		VkFramebuffer get_handle() const;

		const VkExtent2D &get_extent() const;

	private:
		VulkanDevice &device;

		VkFramebuffer handle{VK_NULL_HANDLE};

		VkExtent2D extent{};
	};
} // namespace vkb
