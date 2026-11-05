#include <metal_stdlib>
using namespace metal;


struct FullscreenOut
{
    float4 position [[position]];
    float2 uv;
};


vertex FullscreenOut fullscreen_vertex(uint vertexID [[vertex_id]])
{
    constexpr float2 positions[3] = {
        {-1.0, -1.0},
        { 3.0, -1.0},
        {-1.0,  3.0}
    };

    FullscreenOut out;

    float2 pos = positions[vertexID];

    out.position = float4(pos, 0.0, 1.0);
    out.uv = float2(
        pos.x * 0.5 + 0.5,
        1.0 - (pos.y * 0.5 + 0.5)
    );

    return out;
}


fragment float4 tonemap_fragment(
    FullscreenOut in [[stage_in]],
    texture2d<float> hdrTexture [[texture(0)]]
)
{
    constexpr sampler linearSampler(
        filter::linear,
        address::clamp_to_edge
    );

    float3 hdr = hdrTexture.sample(linearSampler, in.uv).rgb;

    float3 mapped = hdr / (hdr + 1.0);

    return float4(mapped, 1.0);
}