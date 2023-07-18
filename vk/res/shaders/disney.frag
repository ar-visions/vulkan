#version 450

#define MAX_PBR_LIGHTS 1

struct Light {
    vec4 pos;
    vec4 color;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4  model;
    mat4  view;
    mat4  proj;
    vec3  eye;
    Light lights[MAX_PBR_LIGHTS];
} ubo;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in mat3 in_tbn;

layout(location = 0) out vec4 pixel;

layout(binding = 1) uniform sampler2D tx_color;
layout(binding = 2) uniform sampler2D tx_normal;
layout(binding = 3) uniform sampler2D tx_material;


const float Pi = 3.14159265359;

// compute fresnel specular factor for given base specular and product
// product could be NdV or VdH depending on used technique
vec3 fresnel_factor(vec3 f0, float product) {
    return mix(vec3(0), vec3(1.0), pow(1.01 - product, 5.0));
}

float D_beckmann(float roughness, float NdH) {
    float m = roughness * roughness;
    float m2 = m * m;
    float NdH2 = NdH * NdH;
    return exp((NdH2 - 1.0) / (m2 * NdH2)) / (Pi * m2 * NdH2 * NdH2);
}

float G_schlick(float roughness, float NdV, float NdL) {
    float k = roughness * roughness * 0.5;
    float V = NdV * (1.0 - k) + k;
    float L = NdL * (1.0 - k) + k;
    return 0.25 / (V * L);
}

void main() {
    vec3 albedo     = pow(texture(tx_color, in_uv).rgb, vec3(2.2));
    vec3 normal_map = normalize(texture(tx_normal, in_uv).rgb * 2.0 - 1.0);
    vec3 normal     = normalize(in_tbn * normal_map);
    vec3  N         = normal;
    float metallic  = texture(tx_material, in_uv).r * 10.0;
    float roughness = texture(tx_material, in_uv).g * 10.0;
    float ao        = texture(tx_material, in_uv).b;

    // L,V,H vectors
    vec3  L = normalize(ubo.lights[0].pos.xyz - in_pos);
    vec3  V = normalize(ubo.eye - in_pos);
    vec3  H = normalize(L + V);

    float distance    = length(ubo.lights[0].pos.xyz - in_pos);
    float attenuation = 1.0 / (distance * distance);
    vec3  radiance    = ubo.lights[0].color.rgb * attenuation;

    // compute material reflectance
    float NdL = max(0.000, dot(N, L));
    float NdV = max(0.001, dot(N, V));
    float NdH = max(0.001, dot(N, H));
    float HdV = max(0.001, dot(H, V));
    //float LdV = max(0.001, dot(L, V));

    // set constant F0;
    vec3 F0  = vec3(0.04);
    F0       = mix(F0, albedo * ao, metallic);

    float D  = D_beckmann(roughness,NdH);
    vec3  F  = fresnel_factor(F0, HdV);
    float G  = G_schlick(roughness, NdV, NdL);
    vec3  kS = F;
    vec3  kD = vec3(1.0) - kS;
    kD      *= 1.0 - metallic;

    vec3  numerator   = D * G * F;
    float denominator = 4.0 * NdV * NdL;
    vec3  specular    = numerator / max(denominator, 0.001);

    //result color
    vec3 Lo      = (kD * albedo * ao / Pi + specular) * NdL * radiance * 1;
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color   = ambient + Lo;
  
    color        = color / (color + vec3(1.0));
    color        = pow(color, vec3(1.0 / 2.2));

    pixel        = vec4(vec3(D), 1.0);
}