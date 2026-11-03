#pragma once

// Legacy 2D triangle sample: six floats per vertex, 24 bytes.
struct Vertex {
    float x, y;
    float r, g, b, a;
};
static_assert(sizeof(Vertex) == 6 * sizeof(float));

// Coordinates use x/y in [-1, 1] with w = 1.
inline constexpr Vertex triangle_vertices[] = {
    // x     y       r     g     b     a
    { 0.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f }, // sol alt: kirmizi
    { 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f }, // sag alt: yesil
    { 0.0f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f }, // ust: mavi
};
