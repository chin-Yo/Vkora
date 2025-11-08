float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}
//Reduce specular reflection
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
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 evaluateDirectPBR(
    vec3 L,           // light direction (normalized, towards surface)
    vec3 V,           // view direction (normalized, towards camera)
    vec3 N,           // world normal
    vec3 albedo,
    float metallic,
    float roughness,
    vec3 lightColor,
    float lightIntensity
)
{
    vec3 H = normalize(L + V);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    if (dotNL <= 0.0) return vec3(0.0);

    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    // Convert albedo to linear (assuming sRGB input)
    vec3 linearAlbedo = pow(albedo, vec3(2.2));

    // Base reflectance for dielectrics
    vec3 F0 = mix(vec3(0.04), linearAlbedo, metallic);

    // Specular BRDF
    float D = D_GGX(dotNH, roughness);
    float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
    vec3 F = F_Schlick(dotNV, F0);

    vec3 spec = (D * G * F) / (4.0 * dotNL * dotNV + 1e-5);

    // Diffuse (Lambert) scaled by (1 - F) and (1 - metallic)
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * linearAlbedo / PI;

    return (diffuse + spec) * dotNL * lightIntensity * lightColor;
}