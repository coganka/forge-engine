#include "App.hpp"

#include "renderer/Renderer.hpp"
#include "core/Constants.hpp"

#include <iostream>

App::App()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr
            << "SDL init failed: "
            << SDL_GetError()
            << '\n';

        return;
    }

    window = SDL_CreateWindow(
        Config::WindowTitle,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        Config::WindowWidth,
        Config::WindowHeight,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE
    );

    if (!window)
    {
        std::cerr
            << "Window creation failed: "
            << SDL_GetError()
            << '\n';

        return;
    }

    metalView = SDL_Metal_CreateView(window);

    if (!metalView)
    {
        std::cerr
            << "Metal view creation failed: "
            << SDL_GetError()
            << '\n';

        return;
    }

    layer = static_cast<CA::MetalLayer*>(
        SDL_Metal_GetLayer(metalView)
    );

    if (!layer)
    {
        std::cerr << "Could not get CAMetalLayer\n";
        return;
    }

    device = MTL::CreateSystemDefaultDevice();

    if (!device)
    {
        std::cerr << "No Metal device found\n";
        return;
    }

    layer->setDevice(device);

    layer->setPixelFormat(
        MTL::PixelFormatBGRA8Unorm
    );

    renderer = new Renderer(
        device,
        layer
    );

    running = true;

    std::cout
        << "GPU: "
        << device->name()->utf8String()
        << '\n';
}

void App::run()
{
    if (!renderer)
        return;

    while (running)
    {
        processEvents();

        if (!running)
            break;

        renderer->draw();
    }
}

void App::processEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            running = false;
        }

        if (
            event.type == SDL_KEYDOWN &&
            event.key.keysym.sym == SDLK_ESCAPE
        )
        {
            running = false;
        }
    }
}

App::~App()
{
    delete renderer;

    if (metalView)
    {
        SDL_Metal_DestroyView(metalView);
    }

    if (window)
    {
        SDL_DestroyWindow(window);
    }

    SDL_Quit();

    if (device)
    {
        device->release();
    }
}