#include <metal_stdlib>
using namespace metal;


struct VertexIn
{
    float3 position [[attribute(0)]];
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
};


vertex VertexOut unlit_vertex(
    VertexIn in [[stage_in]],
    constant Uniforms& uniforms [[buffer(1)]]
)
{
    VertexOut out;
    
    out.position = 
        uniforms.projection *
        uniforms.view *
        uniforms.model *
        float4(in.position, 1.0);
    
    return out;
}

fragment float4 unlit_fragment()
{
    return float4(0.2, 0.4, 1.0, 1.0);
}