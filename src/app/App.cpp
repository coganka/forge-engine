#include "App.hpp"

#include "renderer/Renderer.hpp"
#include "core/Constants.hpp"

#include "assets/ObjLoader.hpp"
#include "renderer/mesh/Mesh.hpp"
#include <chrono>

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

    scene = std::make_unique<Scene>();

    MeshData data = ObjLoader::load("resources/models/test.obj");

    auto model = std::make_shared<Mesh>(
        device, 
        std::span<const Vertex>(data.vertices),
        std::span<const uint32_t>(data.indices)
    );

    auto& left = scene->addEntity(model);
    left.transform.position[0] = 0.0f;
    left.transform.scale = {0.6f, 0.6f, 0.6f};

    // auto& right = scene->addEntity(model);
    // right.transform.position[0] = 0.8f;
    // right.transform.scale = {0.6f, 0.6f, 0.6f};

    scene->camera().position[2] = -2.0f;

    running = true;

    std::cout
        << "GPU: "
        << device->name()->utf8String()
        << '\n';
}

void App::run()
{
    if (!renderer || !scene)
        return;

    using Clock = std::chrono::steady_clock;
    auto prevTime = Clock::now();

    while (running)
    {
        processEvents();

        if (!running)
            break;

        auto now = Clock::now();
        float deltaTime = std::chrono::duration<float>(now - prevTime).count();
    
        prevTime = now;
        
        scene->update(deltaTime);
        renderer->draw(*scene);
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
    scene.reset();

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