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
    float3 worldPosition;
};

struct Uniforms
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;

    float4 cameraPosition;
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


fragment float4 fragment_main(VertexOut in [[stage_in]], constant Uniforms& uniforms [[buffer(1)]])
{
    float3 N = normalize(in.normal);
    float3 L = normalize(float3(0.5, 1.0, -0.5));
    float3 V = normalize(uniforms.cameraPosition.xyz - in.worldPosition);

    // halfway vector between light and cam
    float3 H = normalize(L + V); 

    float diffuse = max(dot(N, L), 0.0);
    float specular = 0.0f;

    if (diffuse > 0.0)
    {
        specular = pow(max(dot(N, H), 0.0), 32.0);
    }

    float3 ambientColor = in.color * 0.15;
    float3 diffuseColor = in.color * diffuse * 0.7;
    float3 specularColor = float3(1.0) * specular * 0.6;

    float3 finalColor = ambientColor + diffuseColor + specularColor;

    return float4(finalColor, 1.0);
}