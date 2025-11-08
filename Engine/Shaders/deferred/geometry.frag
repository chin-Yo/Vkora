#version 320 es
/* Copyright (c) 2019-2025, Arm Limited and Contributors
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

precision highp float;

// 输入纹理
layout (set = 0, binding = 0) uniform sampler2D base_color_texture;
layout (set = 0, binding = 1) uniform sampler2D metallic_roughness_texture; // ← 新增
layout (set = 0, binding = 2) uniform sampler2D ao_texture;                // ← 可选


layout (location = 0) in vec4 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;

// G-Buffer 输出（3 attachments）
layout (location = 0) out vec4 o_gbuf_albedo;     // .rgb = baseColor (sRGB), .a = AO
layout (location = 1) out vec4 o_gbuf_normal;     // .xyz = normal [0,1], .w = roughness
layout (location = 2) out vec4 o_gbuf_material;   // .r = metallic, .g = emissive (optional)

layout (set = 0, binding = 1) uniform GlobalUniform {
    mat4 model;
    mat4 view_proj;
    vec3 camera_position;
} global_uniform;

layout (push_constant, std430) uniform PBRMaterialUniform {
    vec4 base_color_factor;
    float metallic_factor;
    float roughness_factor;
} pbr_material_uniform;

void main(void)
{
    // === Albedo (Base Color) ===
    vec3 base_color = texture(base_color_texture, in_uv).rgb;
    base_color *= pbr_material_uniform.base_color_factor.rgb;
    // Note: keep in sRGB! Lighting pass will convert to linear.

    // === AO (Ambient Occlusion) ===
    float ao = 1.0;
    #ifdef HAS_AO_TEXTURE
    ao = texture(ao_texture, in_uv).r;
    #endif

    o_gbuf_albedo = vec4(base_color, ao);

    // === Normal ===
    vec3 normal = normalize(in_normal);
    o_gbuf_normal = vec4(normal * 0.5 + 0.5, pbr_material_uniform.roughness_factor);

    // === Material ===
    float metallic = pbr_material_uniform.metallic_factor;
    #ifdef HAS_METALLIC_ROUGHNESS_TEXTURE
    // 常见：metallic in .b, roughness in .g (glTF standard)
    vec4 mr = texture(metallic_roughness_texture, in_uv);
    metallic *= mr.b;                // or .r, depending on export
    // roughness already set above? Or override:
    // float roughness = mr.g;
    // o_gbuf_normal.w = roughness;
    #endif

    o_gbuf_material = vec4(metallic, 0.0, 0.0, 1.0); // .r = metallic, .g = emissive (0 for now)
}
