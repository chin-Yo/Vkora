#version 320 es
precision highp float;

layout (set = 0, binding = 0) uniform sampler2D base_color_texture;
layout (set = 0, binding = 1) uniform sampler2D normal_texture;
layout (set = 0, binding = 2) uniform sampler2D metallic_texture;
layout (set = 0, binding = 3) uniform sampler2D roughness_texture;
layout (set = 0, binding = 4) uniform sampler2D ao_texture;


layout (location = 0) in vec4 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;

// G-Buffer 输出（3 attachments）
layout (location = 0) out vec4 o_gbuf_albedo;     // .rgb = baseColor (sRGB), .a = AO
layout (location = 1) out vec4 o_gbuf_normal;     // .xyz = normal [0,1],
layout (location = 2) out vec4 o_gbuf_material;   // .r = metallic, .g = roughness .b = emissive (optional)

layout (set = 1, binding = 0) uniform GlobalUniform {
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
    ao = texture(ao_texture, in_uv).r;

    o_gbuf_albedo = vec4(base_color, ao);

    // === Normal ===
    vec3 tangent_normal = texture(normal_texture, in_uv).rgb;
    tangent_normal = tangent_normal * 2.0 - 1.0;

    vec3 N = normalize(in_normal);
    vec3 T = normalize(in_tangent);
    T = normalize(T - dot(T, N) * N); 
    vec3 B = cross(N, T);
    
    mat3 TBN = mat3(T, B, N);

    vec3 world_normal = normalize(TBN * tangent_normal);

    o_gbuf_normal = vec4(world_normal * 0.5 + 0.5, 1.0); 

    // === Material ===
    float metallic = pbr_material_uniform.metallic_factor * texture(metallic_texture, in_uv).r;
              
    float roughness = pbr_material_uniform.roughness_factor * texture(roughness_texture, in_uv).r;

    o_gbuf_material = vec4(metallic, roughness, 0.0, 1.0);
}
