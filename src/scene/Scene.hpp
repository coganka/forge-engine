#pragma once

#include <vector>

#include "scene/Camera.hpp"
#include "scene/Entity.hpp"
#include "scene/Light.hpp"


class Scene
{
public:
    Camera& camera() { return camera_; }
    const Camera& camera() const { return camera_; }

    Entity& addEntity(std::shared_ptr<Mesh> mesh);

    void update(float deltaTime);
    Light& addLight(const Light& light);

    const std::vector<Entity>& entities() const
    {
        return entities_;
    }

    const std::vector<Light>& lights() const
    {
        return lights_;
    }

private:
    Camera camera_;
    std::vector<Entity> entities_;
    std::vector<Light> lights_;
};