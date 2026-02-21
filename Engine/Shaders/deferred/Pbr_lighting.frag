#version 450

#include "PbrFun.h"

layout (input_attachment_index = 0, binding = 0) uniform subpassInput i_depth;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput i_albedo;     // .rgb = albedo(sRGB), .a = AO
layout (input_attachment_index = 2, binding = 2) uniform subpassInput i_normal;     // .xyz = normal[0,1]
layout (input_attachment_index = 3, binding = 3) uniform subpassInput i_material;   // .r = metallic, .g = roughness .b = emissive (optional)

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 o_color;

layout (set = 1, binding = 0) uniform GlobalUniform
{
    mat4 inv_view_proj;
    vec2 inv_resolution;
    vec2 padding_0;
    vec4 camPos;// .w ignored
}
global_uniform;

struct Light
{
    vec4 position;         // position.w represents type of light
    vec4 color;            // color.w represents light intensity
    vec4 direction;        // direction.w represents range
    vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

layout (set = 1, binding = 1) uniform LightsInfo
{
    Light directional_lights[48];
    Light point_lights[48];
    Light spot_lights[48];
}
lights_info;

layout (set = 1, binding = 2) uniform samplerCube EnvCube;
layout (set = 1, binding = 3) uniform samplerCube samplerIrradiance;
layout (set = 1, binding = 4) uniform sampler2D samplerBRDFLUT;
layout (set = 1, binding = 5) uniform samplerCube prefilteredMap;

layout (set = 2, binding = 0) uniform sampler2DArray shadowMap;

#define SHADOW_MAP_CASCADE_COUNT 4

layout (set = 2, binding = 1) uniform ShadowUniforms {
    mat4 view;              // 需要视图矩阵来计算 ViewSpace Z 以选择级联
    mat4 cascade_view_proj[SHADOW_MAP_CASCADE_COUNT];
    vec4 cascade_splits;    // .x, .y, .z, .w (存储分段距离/深度)
    int enable_pcf;
    int debug_cascade;      // 1 to show cascade colors
    float _pad1;
    float _pad2;
} shadow_ubo;

// 偏置矩阵用于将 NDC [-1, 1] 映射到 UV [0, 1]
const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0
);

// --- [NEW] 阴影计算函数 ---

// 这里的返回值是 Visibility (1.0 = 亮, 0.0 = 暗)
float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex, float NdotL)
{
    float visibility = 1.0;
    // 根据 NdotL 自适应 Bias，防止阴影痤疮
    float bias = max(0.005 * (1.0 - NdotL), 0.0005);

    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
    {
        // 采样深度
        float dist = texture(shadowMap, vec3(shadowCoord.xy + offset, cascadeIndex)).r;

        // 比较深度 (shadowCoord.w > 0 检查是否在视锥前方)
        // Vulkan 深度通常为 [0, 1]，shadowCoord.z 也是经过 biasMat 变换后的 [0, 1]
        if (shadowCoord.w > 0 && dist < shadowCoord.z - bias)
        {
            visibility = 0.0; // 在阴影中
        }
    }
    return visibility;
}

float filterPCF(vec4 sc, uint cascadeIndex, float NdotL)
{
    ivec2 texDim = textureSize(shadowMap, 0).xy;
    float scale = 0.75; // 控制 PCF 采样半径
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    // 3x3 PCF Kernel
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            shadowFactor += textureProj(sc, vec2(dx * x, dy * y), cascadeIndex, NdotL);
            count++;
        }
    }
    return shadowFactor / float(count);
}

// 计算阴影主函数，返回可见性系数 (0~1) 和级联索引(用于调试)
float calculateShadow(vec3 worldPos, vec3 N, vec3 L, out uint cascadeIndex)
{
    // 1. 计算 View Space Position 用于选择级联
    vec4 viewPos = shadow_ubo.view * vec4(worldPos, 1.0);

    // 2. 选择级联层级
    // 注意：假设 splits 存储的是 View Space Z 值 (通常为负值，如 -10, -50...)
    // 或者 splits 存储的是正的距离 (10, 50...)，取决于 CPU 端设置。
    // 这里参考原代码逻辑：inViewPos.z < ubo.cascadeSplits[i]
    cascadeIndex = 0;
    for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
        if (viewPos.z < shadow_ubo.cascade_splits[i]) {
            cascadeIndex = i + 1;
        }
    }

    // Shadow Coordinate
    // biasMat(NDC → UV) * Proj * View * WorldPos
    /*  [0.5  0.0  0.0  0.5]   [x]   [0.5x + 0.5]
        [0.0  0.5  0.0  0.5] × [y] = [0.5y + 0.5]
        [0.0  0.0  1.0  0.0]   [z]   [z         ]
        [0.0  0.0  0.0  1.0]   [1]   [1         ]*/
    vec4 shadowCoord = (biasMat * shadow_ubo.cascade_view_proj[cascadeIndex]) * vec4(worldPos, 1.0);

    // 透视除法
    vec4 sc_normalized = shadowCoord / shadowCoord.w;

    float NdotL = max(dot(N, L), 0.0);

    // 4. PCF or Simple
    if (shadow_ubo.enable_pcf == 1) {
        return filterPCF(sc_normalized, cascadeIndex, NdotL);
    } else {
        return textureProj(sc_normalized, vec2(0.0), cascadeIndex, NdotL);
    }
}

layout (constant_id = 0) const uint DIRECTIONAL_LIGHT_COUNT = 0U;
layout (constant_id = 1) const uint POINT_LIGHT_COUNT = 0U;
layout (constant_id = 2) const uint SPOT_LIGHT_COUNT = 0U;

float getDistanceAttenuation(float dist, float radius)
{
    // 反平方衰减，带半径截断
    float distSq = dist * dist;
    // 避免除零
    float atten = 1.0 / (distSq + 1.0);
    // 窗口函数：在半径处平滑衰减至0
    float invRadius = 1.0 / max(radius, 0.0001);
    float factor = distSq * invRadius * invRadius;
    float smoothFactor = max(1.0 - factor * factor, 0.0);
    return atten * (smoothFactor * smoothFactor);
}

// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 apply_directional_light(Light light, vec3 pos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, float shadowVisibility)
{
    vec3 L = normalize(-light.direction.xyz);
    // 将阴影可见性乘到光照强度上 (light.color.w) 或者最后的结果上
    return evaluateDirectPBR(L, V, N, albedo, metallic, roughness, light.color.rgb, light.color.w) * shadowVisibility;
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

vec3 getProceduralSky(vec3 viewDir)
{
    vec3 topColor = vec3(0.2, 0.4, 0.8);
    vec3 bottomColor = vec3(0.8, 0.8, 0.8);
    float t = 0.5 * (viewDir.y + 1.0);
    return mix(bottomColor, topColor, t);
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
    const float MAX_REFLECTION_LOD = 7.0; // todo: param/const
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
    vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
    return mix(a, b, lod - lodf);
}

void main()
{
    float depth = subpassLoad(i_depth).x;

    vec4 clip = vec4(in_uv * 2.0 - 1.0, depth, 1.0);
    vec4 world_pos_h = global_uniform.inv_view_proj * clip;
    vec3 world_pos = world_pos_h.xyz / world_pos_h.w;

    vec3 viewDir = normalize(world_pos - global_uniform.camPos.xyz);
    if (depth <= 1e-6) // 使用一点 epsilon 容差
    {
        o_color = vec4(texture(EnvCube, -viewDir).rgb, 1.0);
        return;
    }
    // Load G-Buffer
    vec4 albedo_ao = subpassLoad(i_albedo);
    vec4 normal_rough = subpassLoad(i_normal);
    vec4 material = subpassLoad(i_material);
    float metallic = material.r;

    vec3 albedo = albedo_ao.rgb;          // sRGB
    float ao = albedo_ao.a;               // [0,1]
    vec3 normal = normalize(normal_rough.xyz * 2.0 - 1.0);
    float roughness = material.g;     // [0,1]

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    // View vector (from surface to camera)
    vec3 V = normalize(global_uniform.camPos.xyz - world_pos);
    vec3 R = reflect(-V, -normal);
    // Accumulate direct lighting (HDR)
    vec3 directLight = vec3(0.0);

    uint cascadeIdx = 0;

    // Directional Lights (带阴影)
    for (uint i = 0U; i < DIRECTIONAL_LIGHT_COUNT; ++i)
    {
        float shadowVisibility = 1.0;

        // 通常只对第一个平行光（太阳）计算级联阴影
        if (i == 0) {
            vec3 L = normalize(-lights_info.directional_lights[i].direction.xyz);
            shadowVisibility = calculateShadow(world_pos, normal, L, cascadeIdx);
        }

        directLight += apply_directional_light(
            lights_info.directional_lights[i], world_pos, normal, V, albedo, metallic, roughness, shadowVisibility
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
    vec3 ambient = vec3(0, 0, 0);
    vec2 brdf = texture(samplerBRDFLUT, vec2(max(dot(normal, V), 0.0), roughness)).rg;
    vec3 reflection = prefilteredReflection(R, roughness).rgb;
    vec3 irradiance = texture(samplerIrradiance, -normal).rgb; // normal = world-space
    vec3 indirectDiffuse = albedo * irradiance;
    vec3 F = F_SchlickR(max(dot(normal, V), 0.0), F0, roughness);
    // Specular reflectance
    vec3 specular = reflection * (F * brdf.x + brdf.y);
    // Ambient part
    vec3 kD = 1.0 - F;
    kD *= 1.0 - metallic;
    ambient = (kD * indirectDiffuse + specular) * ao;

    vec3 finalLight = directLight + ambient;
    o_color = vec4(finalLight, 1.0);
}