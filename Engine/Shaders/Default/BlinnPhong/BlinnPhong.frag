#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNormal;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UboScene {
    mat4 projection; // Not used in frag, but part of the UBO
    mat4 view;       // Not used in frag, but part of the UBO
    vec3 cameraPos;
} uboScene;

struct Light {
    vec3 position;
    vec3 color;
    float ambientStrength;
};

layout(set = 1, binding = 0) uniform UboLight {
    Light light;
} uboLight;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

layout(set = 2, binding = 0) uniform UboMaterial {
    Material material;
} uboMaterial;


void main() {
    vec3 norm = normalize(inWorldNormal);
    vec3 viewDir = normalize(uboScene.cameraPos - inWorldPos);
    vec3 lightDir = normalize(uboLight.light.position - inWorldPos);

    vec3 ambient = uboLight.light.ambientStrength * uboLight.light.color * uboMaterial.material.ambient;

    float diffFactor = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffFactor * uboLight.light.color * uboMaterial.material.diffuse;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specFactor = pow(max(dot(norm, halfwayDir), 0.0), uboMaterial.material.shininess);
    vec3 specular = specFactor * uboLight.light.color * uboMaterial.material.specular;
    
    if(diffFactor == 0.0) {
       specular = vec3(0.0, 0.0, 0.0);
    }

    vec3 finalColor = ambient + diffuse + specular;

    outColor = vec4(finalColor, 1.0);
}