#define PI 3.1415926535897932384626433832795

float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 evaluateDirectPBR(
    vec3 L,           // 注意：必须是 "表面指向光源" 的方向 (normalize(LightPos - WorldPos))
    vec3 V,           // View direction (normalized, towards camera)
    vec3 N,           // World normal
    vec3 albedo,
    float metallic,
    float roughness,
    vec3 lightColor,
    float lightIntensity
)
{
    // 1. 确保向量归一化（安全起见）
    L = normalize(L);
    V = normalize(V);
    N = normalize(N);

    vec3 H = normalize(L + V);
    
    // 计算点积
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotHV = clamp(dot(H, V), 0.0, 1.0); // 新增：用于 Fresnel

    // 如果背光，直接返回黑色 (Lambertian Cosine Law)
    if (dotNL <= 0.0) return vec3(0.0);

    // Gamma 校正：通常 Albedo 纹理是 sRGB，需要转 Linear 空间
    vec3 linearAlbedo = pow(albedo, vec3(2.2));

    // F0: 绝缘体为 0.04，金属为 albedo
    vec3 F0 = mix(vec3(0.04), linearAlbedo, metallic);

    // Specular BRDF
    float D = D_GGX(dotNH, roughness);
    float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
    // 修正：这里使用 dotHV 而不是 dotNV
    vec3 F = F_Schlick(dotHV, F0);

    // Cook-Torrance 分母
    vec3 spec = (D * G * F) / (4.0 * dotNL * dotNV + 0.0001); // 使用 0.0001 防止除零

    // Diffuse (Lambert) 
    // 能量守恒：漫反射比例 kD = (1 - F) * (1 - metallic)
    // 注意：对于金属，kD 应该为 0
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    
    vec3 diffuse = kD * linearAlbedo / PI;

    // 最终组合
    // 注意：spec 和 diffuse 已经包含了菲涅尔项的平衡，直接相加
    // dotNL 是光照接收率
    return (diffuse + spec) * dotNL * lightIntensity * lightColor;
}