#version 450

// 输入：模型空间位置
layout(location = 0) in vec3 position;

// Uniform：模型矩阵（TRS）
layout(binding = 0) uniform TransformUBO {
    mat4 modelMatrix; // 完整 TRS 矩阵
} transform;

// 输出到几何着色器
layout(location = 0) out WorldPos {
    vec3 worldPos;
} vs_out;

void main() {
    vs_out.worldPos = (transform.modelMatrix * vec4(position, 1.0)).xyz;
}