#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include <Foundation/Foundation.hpp>
#include "app/App.hpp"


int main()
{
    auto* pool = NS::AutoreleasePool::alloc()->init();

    App app;
    app.run();

    pool->release();

    return 0;
}

