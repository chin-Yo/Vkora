#version 450

// -- 输入 --
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord; // Optional

// -- 输出到片元着色器 --
layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outWorldNormal;

// -- UBO: 每帧更新 --
layout(set = 0, binding = 0) uniform UboScene {
    mat4 projection;
    mat4 view;
} uboScene;

// -- Push Constants: 每个物体更新 --
layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

void main() {
    // 计算世界空间中的顶点位置
    outWorldPos = vec3(pushConstants.model * vec4(inPosition, 1.0));

    // 使用法线矩阵正确变换法线，并传递到片元着色器
    // 法线矩阵是模型矩阵的左上3x3部分的逆转置矩阵
    outWorldNormal = transpose(inverse(mat3(pushConstants.model))) * inNormal;

    // 计算最终的裁剪空间位置
    gl_Position = uboScene.projection * uboScene.view * vec4(outWorldPos, 1.0);
}