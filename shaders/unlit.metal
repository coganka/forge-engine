#include <metal_stdlib>
using namespace metal;

struct VertexIn
{
    float3 position [[attribute(0)]];
    float3 normal [[attribute(3)]];
};


struct Uniforms
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4 cameraPosition;
};


struct VertexOut
{
    float4 position [[position]];
    float3 worldPosition;
    float3 worldNormal;
};


vertex VertexOut
unlit_vertex(VertexIn in [[stage_in]], constant Uniforms& uniforms [[buffer(1)]])
{
    VertexOut out;

    out.position =
        uniforms.projection * uniforms.view * uniforms.model * float4(in.position, 1.0);

    out.worldPosition = (uniforms.model * float4(in.position, 1.0)).xyz;
    out.worldNormal = normalize((uniforms.model * float4(in.normal, 0.0)).xyz);

    return out;
}


fragment float4 unlit_fragment(
    VertexOut in [[stage_in]],
    constant Uniforms& uniforms [[buffer(1)]],
    constant float4& emission [[buffer(4)]]
)
{
    float3 V = normalize(uniforms.cameraPosition.xyz - in.worldPosition);
    float facing = saturate(dot(normalize(in.worldNormal), V));

    float center = pow(facing, 3.0);
    float profile = 0.30 + 0.70 * pow(facing, 1.5);

    float3 coreColor = mix(emission.rgb, sqrt(max(emission.rgb, 0.0)), 0.45 * center);

    return float4(coreColor * emission.a * profile, 1.0);
}
