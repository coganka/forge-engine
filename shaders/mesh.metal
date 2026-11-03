#include <metal_stdlib>

using namespace metal;


struct VertexIn 
{
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
    float2 uv [[attribute(2)]];
    float2 normal [[attribute(3)]];
};

struct VertexOut
{
    float4 position [[position]];
    float3 color;
    float2 uv;
    float3 normal;
};

struct Uniforms
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};


vertex VertexOut vertex_main(VertexIn in [[stage_in]], constant Uniforms& uniforms [[buffer(1)]])
{
    VertexOut out;

    out.position = 
        uniforms.projection *
        uniforms.view *
        uniforms.model * 
        float4(in.position, 1.0);

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


fragment float4 fragment_main(VertexOut in [[stage_in]])
{
    float3 normal = normalize(in.normal);
    float3 lightDir = normalize(float3(0.5, 1.0, -0.5));

    float diffuse = max(dot(normal, lightDir), 0.0);

    float ambient = 0.15;
    float brightness = ambient + diffuse * 0.85;

    return float4(in.color * brightness, 1.0);
}