#include <metal_stdlib>

using namespace metal;


struct VertexIn 
{
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
    float2 uv [[attribute(2)]];
    float3 normal [[attribute(3)]];
};


struct VertexOut
{
    float4 position [[position]];
    float3 color;
    float2 uv;
    float3 normal;
    float3 worldPosition;
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


vertex VertexOut vertex_main(VertexIn in [[stage_in]], constant Uniforms& uniforms [[buffer(1)]])
{
    VertexOut out;

    float4 worldPos = uniforms.model * float4(in.position, 1.0);

    out.worldPosition = worldPos.xyz;

    out.position = 
        uniforms.projection *
        uniforms.view *
        worldPos;

    out.color = in.color;
    out.uv = in.uv;
    out.normal = normalize(
        (uniforms.model * float4(in.normal, 0.0)).xyz
    );

    return out;
}


float checker(float2 uv) 
{
    float2 cell = floor(uv * 8.0);
    return fmod(cell.x + cell.y, 2.0);
}


fragment float4 fragment_main(
    VertexOut in [[stage_in]], 
    constant Uniforms& uniforms [[buffer(1)]],
    constant GPULight* lights [[buffer(2)]],
    constant uint& lightCount [[buffer(3)]]
)
{
    float3 N = normalize(in.normal);
    float3 V = normalize(uniforms.cameraPosition.xyz - in.worldPosition);

    float3 finalColor = in.color * 0.15;

    for (uint i = 0; i < lightCount; ++i) 
    {
        GPULight light = lights[i];

        float3 L;
        float attenuation = 1.0f;

        if (light.type == 0)
        {
            L = normalize(-light.direction.xyz);
        } else if (light.type == 1) 
        {
            float3 toLight = light.position.xyz - in.worldPosition;
            float distanceToLight = length(toLight);

            L = toLight / max(distanceToLight, 0.0001);

            float range = max(light.range, 0.0001);

            float normalizedDistance = distanceToLight / range;

            float falloff = max(
                1.0 - normalizedDistance * normalizedDistance,
                0.0
            );

            attenuation = falloff * falloff;
        } else 
        {
            continue;
        }

        float diffuse = max(dot(N, L), 0.0);
        float3 H = normalize(L + V);
        float specular = 0.0;

        if (diffuse > 0.0)
        {
            specular = pow(max(dot(N, H), 0.0), 32.0);
        }

        float3 radiance = light.color.rgb * light.intensity * attenuation;
        finalColor += in.color * diffuse * radiance * 0.7 + specular * radiance * 0.6;
    }

    return float4(finalColor, 1.0);
}