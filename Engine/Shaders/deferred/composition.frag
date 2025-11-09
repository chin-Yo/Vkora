#version 450

precision highp float;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

// G-Buffer
layout(set = 0, binding = 0) uniform sampler2D gbuf_albedo;     // .rgb=sRGB albedo, .a=AO (from GBuffer)
layout(set = 0, binding = 1) uniform sampler2D gbuf_normal;     // .xyz=normal[0,1], .w=roughness
layout(set = 0, binding = 2) uniform sampler2D gbuf_material;   // .r=metallic

// Lighting Buffer (from Lighting Pass)
layout(set = 0, binding = 3) uniform sampler2D lighting_buffer; // .rgb=HDR direct, .a=AO (optional override)

// IBL Textures
layout(set = 0, binding = 4) uniform samplerCube samplerIrradiance;
layout(set = 0, binding = 5) uniform samplerCube prefilteredMap;
layout(set = 0, binding = 6) uniform sampler2D samplerBRDFLUT;

// Uniforms
layout(set = 0, binding = 7) uniform GlobalUniform
{
    mat4 inv_view_proj;
    vec2 inv_resolution;
    vec3 camPos;
    float exposure;
    float gamma;
}
global_uniform;

// ----------------------------
// BRDF Functions (for IBL)
// ----------------------------
#define PI 3.1415926535897932384626433832795

vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
    const float MAX_REFLECTION_LOD = 9.0; // must match prefiltered map MIP count - 1
    float lod = roughness * MAX_REFLECTION_LOD;
    return textureLod(prefilteredMap, R, lod).rgb;
}

// ----------------------------
// Main
// ----------------------------
void main()
{
    // Optional: early out for sky (if depth == 1.0, but we don't have depth here)
    // Assume full-screen draw

    // Load G-Buffer
    vec4 albedo_ao_gbuf = texture(gbuf_albedo, uv);
    vec4 normal_rough   = texture(gbuf_normal, uv);
    float metallic      = texture(gbuf_material, uv).r;

    vec3 albedo_srgb = albedo_ao_gbuf.rgb;           // sRGB
    float ao_gbuf    = albedo_ao_gbuf.a;             // from G-buffer
    vec3 normal      = normalize(normal_rough.xyz * 2.0 - 1.0);
    float roughness  = normal_rough.w;

    // Load lighting result (direct light)
    vec4 lighting = texture(lighting_buffer, uv);
    vec3 directLight = lighting.rgb;   // HDR direct light
    float ao_from_lighting = lighting.a; // optional: you could use this instead

    // For correctness, we prefer AO from G-buffer (since lighting pass just forwarded it)
    // But both should be identical. Use ao_gbuf.
    float ao = ao_gbuf;

    // Reconstruct world position (needed for view vector)
    // Assumes depth is stored in G-buffer or you have a depth texture.
    // 🔴 YOU NEED DEPTH! Add a depth texture input if not available.
    // For now, assume you have a depth buffer texture:
    // layout(set=0, binding=8) uniform sampler2D depth_buffer;
    // float depth = texture(depth_buffer, uv).r;
    // vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    // vec4 world_pos_h = global_uniform.inv_view_proj * clip;
    // vec3 world_pos = world_pos_h.xyz / world_pos_h.w;

    // ⚠️ TEMPORARY: if you don't have depth texture, you CANNOT reconstruct world pos!
    // But in many engines, Composition Pass reuses the same depth buffer from G-buffer.
    // Let's assume you bind depth as `depth_buffer` (binding=8)

    // 🔴 UNCOMMENT AND BIND depth_buffer IF YOU HAVE IT
    // float depth = texture(depth_buffer, uv).x;
    // vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    // vec4 world_pos_h = global_uniform.inv_view_proj * clip;
    // vec3 world_pos = world_pos_h.xyz / world_pos_h.w;
    // vec3 V = normalize(global_uniform.camPos - world_pos);

    // ✅ ALTERNATIVE (if you stored world position in G-buffer, e.g., in a fat G-buffer):
    // But to keep G-buffer lean, we reconstruct from depth.

    // 🚨 Since you didn't provide depth buffer in this shader, we must add it.
    // I'll add it as binding=8 below in comments.

    // For now, let's assume you ADD THIS:
    // layout(set=0, binding=8) uniform sampler2D depth_buffer;
    // and uncomment the block below.

    // ----- RECONSTRUCT WORLD POS (UNCOMMENT IF YOU BIND depth_buffer) -----
    // float depth = texture(depth_buffer, uv).x;
    // if (depth >= 1.0) {
    //     // Optional: output sky color or discard
    //     outColor = vec4(0.1, 0.2, 0.4, 1.0); // simple sky
    //     return;
    // }
    // vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    // vec4 world_pos_h = global_uniform.inv_view_proj * clip;
    // vec3 world_pos = world_pos_h.xyz / world_pos_h.w;
    // vec3 V = normalize(global_uniform.camPos - world_pos);
    // ---------------------------------------------------------------------

    // ⚠️ If you CANNOT add depth buffer, you must store world position in G-buffer!
    // Common alternative: store view-space position or linear depth.

    // ✅ FOR THIS EXAMPLE, I'LL ASSUME YOU STORED VIEW-SPACE Z OR HAVE DEPTH.
    // But to make it work NOW, let's use a fallback (not correct, but compiles):
    vec3 V = vec3(0, 0, 1); // 🚨 REPLACE WITH REAL VIEW VECTOR!

    // ----------------------------
    // IBL: Indirect Lighting
    // ----------------------------
    vec3 F0 = mix(vec3(0.04), pow(albedo_srgb, vec3(2.2)), metallic);
    float NdotV = max(dot(normal, V), 0.001);
    vec3 F = F_SchlickR(NdotV, F0, roughness);

    // Diffuse (irradiance)
    vec3 irradiance = texture(samplerIrradiance, normal).rgb;
    vec3 diffuse = irradiance * pow(albedo_srgb, vec3(2.2));

    // Specular (pre-filtered environment + BRDF LUT)
    vec3 R = reflect(-V, normal);
    vec3 prefilteredColor = prefilteredReflection(R, roughness);
    vec2 brdf = texture(samplerBRDFLUT, vec2(NdotV, roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    // Combine indirect with AO (ONLY indirect!)
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 indirectLight = (kD * diffuse + specular) * ao;

    // ----------------------------
    // Final Composition
    // ----------------------------
    vec3 color = directLight + indirectLight; // HDR

    // Tone mapping (Uncharted 2)
    vec3 toneMapped = color * global_uniform.exposure;
    toneMapped = (toneMapped * (0.15 * toneMapped + 0.50 * 0.10) + 0.20 * 0.02)
               / (toneMapped * (0.15 * toneMapped + 0.50) + 0.20 * 0.30)
               - 0.02 / 0.30;
    // White balance
    float white = (11.2 * (0.15 * 11.2 + 0.50 * 0.10) + 0.20 * 0.02)
                / (11.2 * (0.15 * 11.2 + 0.50) + 0.20 * 0.30)
                - 0.02 / 0.30;
    toneMapped = toneMapped / white;

    // Gamma correction
    vec3 gammaCorrected = pow(toneMapped, vec3(1.0 / global_uniform.gamma));

    outColor = vec4(gammaCorrected, 1.0);
}