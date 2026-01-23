#version 450

#define LIGHT_COUNT 3
#define FACE_COUNT 6

layout(triangles) in;
layout(triangle_strip, max_vertices = LIGHT_COUNT * FACE_COUNT * 3) out;

// 输入：来自 VS 的世界空间位置
layout(location = 0) in WorldPos {
    vec3 worldPos;
} gs_in[];

// 输出：传递给 FS（可选，用于深度优化或可视化）
// layout(location = 0) out FragData {
//     vec3 worldPos;
// } gs_out;

// === Uniforms ===

// 光源位置（用于调试或 fallback）
layout(binding = 1) uniform LightPosUBO {
    vec3 lightPositions[LIGHT_COUNT];
} lightPos;

// 预计算的 Shadow View-Projection 矩阵: [light][face]
layout(binding = 2) uniform ShadowMatricesUBO {
    mat4 shadowViewProj[LIGHT_COUNT][FACE_COUNT];
} shadowMats;

// ——————————————————————————————
// 主函数
// ——————————————————————————————
void main() {
    // 遍历每个光源
    for (int lightIdx = 0; lightIdx < LIGHT_COUNT; ++lightIdx) {
        // 遍历 6 个立方体贴图面
        for (int face = 0; face < FACE_COUNT; ++face) {
            // 设置输出图层：每个光源占连续 6 层
            gl_Layer = lightIdx * FACE_COUNT + face;

            // 发射三角形的三个顶点
            for (int i = 0; i < 3; ++i) {
                vec3 wp = gs_in[i].worldPos;
                //gs_out.worldPos = wp;

                // 使用预计算的 Shadow VP 矩阵
                gl_Position = shadowMats.shadowViewProj[lightIdx][face] * vec4(wp, 1.0);

                EmitVertex();
            }
            EndPrimitive();
        }
    }
}