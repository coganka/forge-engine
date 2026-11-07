#include <metal_stdlib>
using namespace metal;

struct FullscreenOut
{
    float4 position [[position]];
    float2 uv;
};

vertex FullscreenOut bloom_vertex(uint id [[vertex_id]])
{
    float2 positions[3] = {
        {-1.0, -1.0},
        { 3.0, -1.0},
        {-1.0,  3.0}
    };

    FullscreenOut out;
    float2 pos = positions[id];

    out.position = float4(pos, 0.0, 1.0);
    out.uv = float2(
        pos.x * 0.5 + 0.5,
        1.0 - (pos.y * 0.5 + 0.5)
    );

    return out;
}

fragment float4 bloom_extract(
    FullscreenOut in [[stage_in]],
    texture2d<float> hdr [[texture(0)]]
)
{
    constexpr sampler s(
        filter::linear,
        address::clamp_to_edge
    );

    float3 color = hdr.sample(s, in.uv).rgb;

    float brightness = max(
        color.r,
        max(color.g, color.b)
    );

    float mask = smoothstep(0.8, 1.5, brightness);

    return float4(color * mask, 1.0);
}