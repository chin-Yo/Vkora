#version 450
// 开启扩展，允许在顶点着色器中写入 gl_Layer
#extension GL_ARB_shader_viewport_layer_array : enable 

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord_0; // 如果需要 Alpha Test

// 假设有 4 级级联
#define CASCADE_COUNT 4

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 viewProj[CASCADE_COUNT]; // 4 个级联的 VP 矩阵
} ubo;

layout(push_constant) uniform PushConsts {
    mat4 model; // 物体的模型矩阵
} push;

layout(location = 0) out vec2 outUV;

void main() {
    // gl_InstanceIndex 对应当前的级联索引 (0, 1, 2, 3)
    int cascadeIndex = gl_InstanceIndex;

    // 1. 计算世界坐标
    vec4 worldPos = push.model * vec4(position, 1.0);

    // 2. 选择对应级联的 VP 矩阵进行变换
    gl_Position = ubo.viewProj[cascadeIndex] * worldPos;

    // 3. 【关键】直接指定渲染到 Framebuffer 的哪一层
    gl_Layer = cascadeIndex;

    // 4. 传递 UV 供片元着色器做 Alpha Test
    outUV = texcoord_0;
}