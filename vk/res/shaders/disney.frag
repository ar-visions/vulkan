#version 450

#define MAX_PBR_LIGHTS 3

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 light_pos [MAX_PBR_LIGHTS];
    vec4 light_rgba[MAX_PBR_LIGHTS];
} ubo;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 pixel;

#define ASSET_COUNT 3

#define COLOR    0
#define NORMAL   1
#define MATERIAL 2

layout(binding  = 1) uniform sampler2D tx[ASSET_COUNT];

void main() {
    vec3  color       = texture(tx[COLOR], in_uv).rgb;
    vec3  normal      = normalize(texture(tx[NORMAL], in_uv).rgb * 2.0 - 1.0);
    vec3  material    = texture(tx[MATERIAL], in_uv).rgb;
    float metallic    = material.r;
    float roughness   = material.g;
    float ao          = material.b;
    vec3  N           = normalize( in_normal);
    vec3  V           = normalize(-in_pos);
    vec3  total_light = vec3(0.0);

    for (int i = 0; i < MAX_PBR_LIGHTS; ++i) {
        vec3  light    = ubo.light_rgba[i].rgb * ubo.light_rgba[i].a;

        vec3  L        = normalize(ubo.light_pos[i] - in_pos);
        vec3  H        = normalize(V + L);

        float NdotL    = max(dot(N, L), 0.0);
        float NdotV    = max(dot(N, V), 0.0);
        float NdotH    = max(dot(N, H), 0.0);
        float LdotH    = max(dot(L, H), 0.0);

        vec3  diffuse  = color * (1.0 - metallic);
        vec3  specular = vec3(0.04) + (color - vec3(0.04)) * pow(1.0 - NdotV, 5.0);
        vec3  f0       = mix(vec3(0.04), color, metallic);
        vec3  f90      = vec3(1.0);
        float D        = exp(-pow(roughness, 2.0) / (2.0 * roughness * roughness * NdotH + 0.001));
        float G        = min(1.0, min(2.0 * NdotH * NdotV / LdotH, 2.0 * NdotH * NdotL / LdotH));
        vec3  F        = f0 + (f90 - f0) * pow(1.0 - LdotH, 5.0);

        vec3 specularBRDF = (D * G * F) / (4.0 * NdotL * NdotV + 0.001);
        vec3 diffuseBRDF  = (1.0 - specularBRDF) * diffuse / (3.14159 + 0.001);
        
        total_light   += (diffuseBRDF + specularBRDF * specular) * light;
    }

    vec3 ambient       = color * ao;
    vec3 final_color   = ambient + total_light;
    pixel              = vec4(final_color, 1.0);
}