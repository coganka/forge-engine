#pragma once

#include <SDL.h>
#include <SDL_metal.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

class Renderer;

class App
{
public:
    App();
    ~App();

    void run();

private:
    SDL_Window* window = nullptr;
    SDL_MetalView metalView = nullptr;

    MTL::Device* device = nullptr;
    CA::MetalLayer* layer = nullptr;

    Renderer* renderer = nullptr;

    bool running = false;

    void processEvents();
};