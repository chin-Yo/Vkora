#include "Core/Math/MathUtils.h"

glm::vec3 MathUtils::EulerToDirection(const glm::vec3& euler)
{
    float yaw = euler.y; // 绕 Y 轴（左右）
    float pitch = euler.x; // 绕 X 轴（上下）
    // euler.z (roll) 被忽略

    float cosYaw = cos(yaw);
    float sinYaw = sin(yaw);
    float cosPitch = cos(pitch);
    float sinPitch = sin(pitch);

    glm::vec3 direction(
        sinYaw * cosPitch,
        sinPitch,
        -cosYaw * cosPitch // 负号：默认朝向为 -Z
    );

    return direction; // 数学上已归一化，通常无需 normalize()
}
