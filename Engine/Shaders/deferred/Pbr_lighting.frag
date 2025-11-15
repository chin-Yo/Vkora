#version 450
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

layout (input_attachment_index = 0, binding = 0) uniform subpassInput i_depth;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput i_albedo;    // .rgb = albedo(sRGB), .a = AO
layout (input_attachment_index = 2, binding = 2) uniform subpassInput i_normal;    // .xyz = normal[0,1]
layout (input_attachment_index = 3, binding = 3) uniform subpassInput i_material;// .r = metallic, .g = roughness .b = emissive (optional)

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 o_color;

#define PI 3.1415926535897932384626433832795

#include "Common/PbrFun.h"

layout (set = 0, binding = 3) uniform GlobalUniform
{
    mat4 inv_view_proj;
    vec2 inv_resolution;
}
global_uniform;


struct Light
{
    vec4 position;         // position.w represents type of light
    vec4 color;            // color.w represents light intensity
    vec4 direction;        // direction.w represents range
    vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

vec3 apply_directional_light(Light light, vec3 pos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
    vec3 L = normalize(-light.direction.xyz); // light direction toward surface
    return evaluateDirectPBR(L, V, N, albedo, metallic, roughness, light.color.rgb, light.color.w);
}

vec3 apply_point_light(Light light, vec3 pos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
    vec3 L_vec = light.position.xyz - pos;
    float dist = length(L_vec);
    if (dist <= 0.01) return vec3(0.0);

    vec3 L = L_vec / dist;

    // Quadratic attenuation with range cutoff
    float range = light.direction.w; // max influence radius
    float atten = 1.0 / (1.0 + 0.09 * dist + 0.03 * dist * dist); // UE4-style
    // Optional hard cutoff
    if (dist > range) atten = 0.0;

    return evaluateDirectPBR(L, V, N, albedo, metallic, roughness, light.color.rgb, light.color.w * atten);
}

vec3 apply_spot_light(Light light, vec3 pos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
    vec3 L_vec = light.position.xyz - pos;
    float dist = length(L_vec);
    if (dist <= 0.01) return vec3(0.0);

    vec3 L = L_vec / dist;

    // Attenuation
    float range = light.direction.w;
    float atten = 1.0 / (1.0 + 0.09 * dist + 0.03 * dist * dist);
    if (dist > range) atten = 0.0;

    // Spot cone (cosθ space)
    vec3 spotDir = normalize(light.direction.xyz);
    float cosTheta = dot(-L, spotDir); // angle between light dir and pixel
    float outerConeCos = light.info.y;
    float innerConeCos = light.info.x;

    // Smooth cone falloff
    float spotEffect = 0.0;
    if (cosTheta > outerConeCos)
    {
        spotEffect = smoothstep(outerConeCos, innerConeCos, cosTheta);
        atten *= spotEffect;
    }
    else
    {
        return vec3(0.0);
    }

    return evaluateDirectPBR(L, V, N, albedo, metallic, roughness, light.color.rgb, light.color.w * atten);
}

layout (set = 0, binding = 4) uniform LightsInfo
{
    Light directional_lights[48];
    Light point_lights[48];
    Light spot_lights[48];
}
lights_info;

layout (constant_id = 0) const uint DIRECTIONAL_LIGHT_COUNT = 0U;
layout (constant_id = 1) const uint POINT_LIGHT_COUNT = 0U;
layout (constant_id = 2) const uint SPOT_LIGHT_COUNT = 0U;

void main()
{
    // --- Reconstruct world position from depth ---
    float depth = subpassLoad(i_depth).x;
    if (depth >= 1.0) discard; // optional: skip sky

    vec4 clip = vec4(in_uv * 2.0 - 1.0, depth, 1.0);
    vec4 world_pos_h = global_uniform.inv_view_proj * clip;
    vec3 world_pos = world_pos_h.xyz / world_pos_h.w;

    // Load G-Buffer
    vec4 albedo_ao = subpassLoad(i_albedo);
    vec4 normal_rough = subpassLoad(i_normal);
    float metallic = subpassLoad(i_material).r;

    vec3 albedo = albedo_ao.rgb;          // sRGB
    float ao = albedo_ao.a;               // [0,1]
    vec3 normal = normalize(normal_rough.xyz * 2.0 - 1.0);
    float roughness = normal_rough.w;     // [0,1]

    // View vector (from surface to camera)
    vec3 V = normalize(global_uniform.camPos - world_pos);

    // Accumulate direct lighting (HDR)
    vec3 directLight = vec3(0.0);

    for (uint i = 0U; i < DIRECTIONAL_LIGHT_COUNT; ++i)
    {
        directLight += apply_directional_light(
            lights_info.directional_lights[i], world_pos, normal, V, albedo, metallic, roughness
        );
    }
    for (uint i = 0U; i < POINT_LIGHT_COUNT; ++i)
    {
        directLight += apply_point_light(
            lights_info.point_lights[i], world_pos, normal, V, albedo, metallic, roughness
        );
    }
    for (uint i = 0U; i < SPOT_LIGHT_COUNT; ++i)
    {
        directLight += apply_spot_light(
            lights_info.spot_lights[i], world_pos, normal, V, albedo, metallic, roughness
        );
    }

    // Output: HDR direct light in RGB, AO in alpha (for composition pass)
    o_color = vec4(directLight, ao);
}