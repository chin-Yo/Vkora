#version 450
// Enable extension to allow writing to gl_Layer in vertex shader
#extension GL_ARB_shader_viewport_layer_array: enable 

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord_0; // For Alpha Test if needed

#define CASCADE_COUNT 4

layout (set = 0, binding = 0) uniform GlobalUBO {
    mat4 viewProj[CASCADE_COUNT];
} ubo;

layout (push_constant) uniform PushConsts {
    mat4 model;
} push;

layout (location = 0) out vec2 outUV;

void main() {
    // gl_InstanceIndex corresponds to the current cascade index (0, 1, 2, 3)
    int cascadeIndex = gl_InstanceIndex;

    // Calculate world position
    vec4 worldPos = push.model * vec4(position, 1.0);

    // Select the corresponding cascade's VP matrix for transformation
    gl_Position = ubo.viewProj[cascadeIndex] * worldPos;

    // [Key] Directly specify which layer of the Framebuffer to render to
    gl_Layer = cascadeIndex;

    // Pass UV coordinates for Alpha Test in fragment shader
    outUV = texcoord_0;
}