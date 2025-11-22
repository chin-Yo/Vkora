#version 320 es
/* Copyright (c) 2019, Arm Limited and Contributors
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

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord_0;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 color;
layout(location = 4) in vec3 tangent;

layout(set = 1, binding = 0) uniform GlobalUniform {
    mat4 model;
    mat4 view_proj;
    vec3 camera_position;
} global_uniform;

layout (location = 0) out vec4 o_pos;
layout (location = 1) out vec2 o_uv;
layout (location = 2) out vec3 o_normal;
layout (location = 3) out vec3 o_tangent;

void main(void)
{
    o_pos = global_uniform.model * vec4(position, 1.0);

    o_uv = texcoord_0;

    mat3 model_3x3 = mat3(global_uniform.model);

    o_normal = model_3x3 * normal;

    o_tangent = model_3x3 * tangent.xyz;

    gl_Position = global_uniform.view_proj * o_pos;
}
