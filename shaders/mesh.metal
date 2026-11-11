#include <metal_stdlib>
using namespace metal;

struct VertexIn
{
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
    float2 uv [[attribute(2)]];
    float3 normal [[attribute(3)]];
};


struct Uniforms
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4 cameraPosition;
};


struct GPULight
{
    float4 position;
    float4 direction;
    float4 color;
    float intensity;
    float range;
    float innerConeCos;
    float outerConeCos;
    uint type;
};


struct MaterialParams
{
    float4 baseColor;
    float4 emissive;
    float metallic;
    float roughness;
    float normalStrength;
    float separateRoughness;
    float4 uvTransform;
};


struct ShadowData
{
    float4x4 viewProjection;
    float4 settings;
};


struct VertexOut
{
    float4 position [[position]];
    float3 color;
    float2 uv;
    float3 normal;
    float3 worldPosition;
    float4 shadowPosition;
};


vertex float4 shadow_vertex(VertexIn in [[stage_in]], constant float4x4& mvp [[buffer(1)]])
{
    return mvp * float4(in.position, 1);
}


vertex VertexOut vertex_main(
    VertexIn in [[stage_in]],
    constant Uniforms& u [[buffer(1)]],
    constant float4x4& normalMatrix [[buffer(5)]],
    constant ShadowData& shadow [[buffer(6)]]
)
{
    VertexOut out;

    float4 world = u.model * float4(in.position, 1);

    out.position = u.projection * u.view * world;

    out.worldPosition = world.xyz;

    out.normal = normalize((normalMatrix * float4(in.normal, 0)).xyz);

    out.uv = in.uv;
    out.color = in.color;

    out.shadowPosition = shadow.viewProjection * world;

    return out;
}


float visibility(
    float4 position, float ndotl, constant ShadowData& shadow, depth2d<float> map
)
{
    float3 p = position.xyz / position.w;
    float2 uv = float2(p.x * 0.5 + 0.5, 0.5 - p.y * 0.5);

    if (shadow.settings.w == 0 || any(uv < 0) || any(uv > 1) || p.z <= 0 || p.z >= 1)
    {
        return 1;
    }

    constexpr sampler s(
        coord::normalized,
        address::clamp_to_edge,
        filter::linear,
        compare_func::less_equal
    );

    float bias = shadow.settings.y * (1 + 3 * (1 - ndotl));
    float result = 0;

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            result += map.sample_compare(
                s, uv + float2(x, y) * shadow.settings.z, p.z - bias
            );
        }
    }

    return result / 9;
}


fragment float4 fragment_main(
    VertexOut in [[stage_in]],
    constant Uniforms& u [[buffer(1)]],
    constant GPULight* lights [[buffer(2)]],
    constant uint& count [[buffer(3)]],
    constant MaterialParams& m [[buffer(4)]],
    constant ShadowData& shadow [[buffer(6)]],
    texture2d<float> baseMap [[texture(0)]],
    texture2d<float> normalMap [[texture(1)]],
    texture2d<float> mrMap [[texture(2)]],
    texture2d<float> emissiveMap [[texture(3)]],
    depth2d<float> shadowMap [[texture(4)]],
    texture2d<float> roughnessMap [[texture(5)]]
)
{
    constexpr sampler s(
        filter::linear, mip_filter::linear, address::repeat, max_anisotropy(8)
    );

    float2 uv = in.uv * m.uvTransform.xy + m.uvTransform.zw;
    float3 albedo = baseMap.sample(s, uv).rgb * m.baseColor.rgb * in.color;
    float3 N = normalize(in.normal);

    float3 dp1 = dfdx(in.worldPosition);
    float3 dp2 = dfdy(in.worldPosition);
    float2 dt1 = dfdx(uv);
    float2 dt2 = dfdy(uv);
    float3 p2 = cross(dp2, N);
    float3 p1 = cross(N, dp1);
    float3 T = p2 * dt1.x + p1 * dt2.x;
    float3 B = p2 * dt1.y + p1 * dt2.y;

    float denom = max(dot(T, T), dot(B, B));

    float3 sampled = normalMap.sample(s, uv).xyz * 2 - 1;
    sampled.xy *= m.normalStrength;

    if (denom > 1e-12)
    {
        N = normalize(float3x3(T * rsqrt(denom), B * rsqrt(denom), N) * sampled);
    }

    float3 mr = mrMap.sample(s, uv).rgb;
    float metallic = saturate(m.metallic * mr.b);
    float roughnessSample =
        mix(mr.g, roughnessMap.sample(s, uv).r, m.separateRoughness);
    float roughness = clamp(m.roughness * roughnessSample, 0.06, 1.0);

    float3 V = normalize(u.cameraPosition.xyz - in.worldPosition);
    float nv = max(dot(N, V), 0.001);
    float3 f0 = mix(float3(0.04), albedo, metallic);

    float3 result = float3(0.0);

    for (uint i = 0; i < count; ++i)
    {
        GPULight light = lights[i];

        float3 L;
        float attenuation = 1;

        if (light.type == 0)
        {
            L = normalize(-light.direction.xyz);
        }
        else
        {
            float3 delta = light.position.xyz - in.worldPosition;
            float d = max(length(delta), 0.001);
            L = delta / d;

            float falloff = saturate(1 - pow(d / max(light.range, 0.001), 4.0));
            attenuation = falloff * falloff / max(d * d, 0.04);

            if (light.type == 2)
            {
                attenuation *= smoothstep(
                    light.outerConeCos,
                    light.innerConeCos,
                    dot(-L, normalize(light.direction.xyz))
                );
            }
        }

        float nl = max(dot(N, L), 0.0);
        float3 H = normalize(V + L);
        float nh = max(dot(N, H), 0.0);
        float vh = max(dot(V, H), 0.0);

        float a = roughness * roughness;
        float a2 = a * a;
        float d = nh * nh * (a2 - 1) + 1;
        float D = a2 / max(M_PI_F * d * d, 1e-6);

        float k = (roughness + 1) * (roughness + 1) / 8;
        float G = (nv / (nv * (1 - k) + k)) * (nl / (nl * (1 - k) + k));

        float3 F = f0 + (1 - f0) * pow(1 - vh, 5.0);

        float3 spec = D * G * F / max(4 * nv * nl, 0.001);
        float3 diffuse = (1 - F) * (1 - metallic) * albedo / M_PI_F;

        float vis = (int(i) == int(shadow.settings.x))
                        ? visibility(in.shadowPosition, nl, shadow, shadowMap)
                        : 1;

        result += (diffuse + spec) * light.color.rgb * light.intensity * attenuation *
                  nl * vis;
    }

    result += emissiveMap.sample(s, uv).rgb * m.emissive.rgb;

    return float4(result, 1);
}
