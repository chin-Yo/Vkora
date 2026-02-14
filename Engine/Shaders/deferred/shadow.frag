#version 450
void main() {
    // 强制所有片段深度为 0.5（中间深度）
    gl_FragDepth = 0.5;
    fragColor = vec4(1.0, 0.0, 0.0, 1.0); // 红色表示“强制深度”
}