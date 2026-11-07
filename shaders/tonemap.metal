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
    texture2d<float> hdrTexture [[texture(0)]],
    texture2d<float> bloomTexture [[texture(1)]]
)
{
    constexpr sampler s(
        filter::linear,
        address::clamp_to_edge
    );

    // Original HDR scene
    float3 hdr = hdrTexture.sample(s, in.uv).rgb;

    // One texel in normalized UV coordinates
    float2 texelSize = 1.0 / float2(
        bloomTexture.get_width(),
        bloomTexture.get_height()
    );

    // Gaussian-like weights
    const float weights[5] = {
        1.0, 4.0, 6.0, 4.0, 1.0
    };

    float3 bloom = float3(0.0);

    // Blur bright pixels into their neighbors
    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            float2 offset =
                float2(float(x), float(y))
                * texelSize * 4.0;

            float weight =
                weights[x + 2] *
                weights[y + 2];

            bloom += bloomTexture.sample(
                s,
                in.uv + offset
            ).rgb * weight;
        }
    }

    // Normalize blur weights: (1+4+6+4+1)^2 = 256
    bloom /= 256.0;

    // Combine original scene and blurred brightness
    float bloomStrength = 1.5;

    float3 combined = hdr + bloom * bloomStrength;

    // Reinhard tone mapping
    float3 mapped = combined / (combined + 1.0);

    return float4(mapped, 1.0);
}