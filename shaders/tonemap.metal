#include <metal_stdlib>
using namespace metal;

struct FullscreenOut
{
    float4 position [[position]];
    float2 uv;
};


vertex FullscreenOut fullscreen_vertex(uint vertexID [[vertex_id]])
{
    constexpr float2 positions[3] = {{-1.0, -1.0}, {3.0, -1.0}, {-1.0, 3.0}};

    FullscreenOut out;

    float2 pos = positions[vertexID];

    out.position = float4(pos, 0.0, 1.0);
    out.uv = float2(pos.x * 0.5 + 0.5, 1.0 - (pos.y * 0.5 + 0.5));

    return out;
}


fragment float4 tonemap_fragment(
    FullscreenOut in [[stage_in]],
    texture2d<float> hdrTexture [[texture(0)]],
    texture2d<float> bloom0 [[texture(1)]],
    texture2d<float> bloom1 [[texture(2)]],
    texture2d<float> bloom2 [[texture(3)]],
    texture2d<float> bloom3 [[texture(4)]]
)
{
    constexpr sampler s(filter::linear, address::clamp_to_edge);

    float3 hdr = hdrTexture.sample(s, in.uv).rgb;
    float3 bloom =
        bloom0.sample(s, in.uv).rgb * 0.36 + bloom1.sample(s, in.uv).rgb * 0.30 +
        bloom2.sample(s, in.uv).rgb * 0.22 + bloom3.sample(s, in.uv).rgb * 0.12;
    constexpr float bloomStrength = 0.9;
    float3 combined = max(hdr + bloom * bloomStrength, 0.0);

    float peak = max(combined.r, max(combined.g, combined.b));
    float3 mapped = combined / (1.0 + peak);

    float3 srgb = select(
        1.055 * pow(mapped, float3(1.0 / 2.4)) - 0.055,
        mapped * 12.92,
        mapped <= 0.0031308
    );

    return float4(srgb, 1.0);
}
