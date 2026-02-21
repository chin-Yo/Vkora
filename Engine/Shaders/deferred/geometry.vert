#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord_0;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 color;
layout (location = 4) in vec3 tangent;

layout (set = 1, binding = 0) uniform GlobalUniform {
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
    // Transform vertex position to world space using the model matrix
    o_pos = global_uniform.model * vec4(position, 1.0);

    // Flip the Y-coordinate of UV to match texture coordinate convention
    o_uv = vec2(texcoord_0.x, 1.0 - texcoord_0.y);

    // Extract the upper-left 3x3 portion of the model matrix for normal/tangent transformation
    mat3 model_3x3 = mat3(global_uniform.model);

    // Transform normal vector to world space using the 3x3 model matrix
    o_normal = model_3x3 * normal;

    // Transform tangent vector to world space using the 3x3 model matrix
    o_tangent = model_3x3 * tangent.xyz;

    // Transform world-space position to clip space using the view-projection matrix
    gl_Position = global_uniform.view_proj * o_pos;
}

