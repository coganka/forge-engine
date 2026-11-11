# Forge Engine

A small game engine in progress, built with C++20 and Apple's Metal API on macOS.

I'm building Forge to learn how rendering and engine systems work beneath the API. It started with graphics fundamentals and is growing through research, experiments, and small features. I work on it in my spare time, so the code and architecture will keep changing as I learn.

## Current work

- Metal rendering with vertex and index buffers, shaders, and depth testing.
- Model, view, and projection transforms with a perspective camera.
- OBJ mesh loading with position, UV, and normal data.
- Textured metallic/roughness materials, normal maps, GGX lighting, HDR and bloom.
- Directional shadow mapping; point-light shadows are not implemented yet.
- A simple scene and entity structure with per-entity transforms and shared meshes.

The focus is on understanding the math, data flow, and resource management behind each feature, then measuring and improving the implementation.

## Where it's heading

The goal is a compact engine that can run a small interactive 3D scene with convincing lighting and materials. Planned areas include environment lighting, point-light shadows, input, and better scene management. I also want to explore custom memory allocation, CPU/GPU synchronization, and profiling as the project grows.

This is an ongoing learning project, with plenty still to implement and refine.

## Building

Requires macOS with Metal support, Xcode's C++ and Metal toolchains, SDL2, and [metal-cpp](https://developer.apple.com/metal/cpp/).

Install SDL2 and place metal-cpp in `vendor/metal-cpp`:

```sh
brew install sdl2
mkdir -p vendor
git clone https://github.com/apple/metal-cpp.git vendor/metal-cpp
```

The test scene uses local model and texture files that are not included in the repository. Run from the project root:

```sh
make run
```

Downloaded dependencies, model files, and build output are excluded from Git. Source and license notes are kept in [Lantern credits](resources/models/lantern/SOURCE.md) and [paving credits](resources/textures/paving-stones/SOURCE.md).

## Textured material test

`make run` opens the CC0 Lantern scene with a normal-mapped stone paving ground, normal mapping, metallic/roughness lighting, emissive bloom and a warm point light inside the stationary lantern. The background is black and ambient fill is disabled. Directional PCF shadows remain available when a directional light is added. Scene settings live in `src/scene/TestScene.cpp`.
