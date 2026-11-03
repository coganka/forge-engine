#pragma once

#include <vector>

#include "scene/Camera.hpp"
#include "scene/Entity.hpp"


class Scene
{
public:
    Camera& camera() { return camera_; }
    const Camera& camera() const { return camera_; }

    Entity& addEntity(std::shared_ptr<Mesh> mesh);

    void update(float deltaTime);

    const std::vector<Entity>& entities() const
    {
        return entities_;
    }

private:
    Camera camera_;
    std::vector<Entity> entities_;
};