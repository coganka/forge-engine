#pragma once

#include <vector>
#include <deque>
#include <string>


#include "scene/Camera.hpp"
#include "scene/Entity.hpp"
#include "scene/Light.hpp"


class Scene
{
public:
    Camera& camera()
    {
        return camera_;
    }

    const Camera& camera() const
    {
        return camera_;
    }

    Entity&
    addEntity(std::string name, std::shared_ptr<Mesh> mesh, Material material = {});

    void update(float deltaTime);
    Light& addLight(const Light& light);

    const std::deque<Entity>& entities() const
    {
        return entities_;
    }

    const std::vector<Light>& lights() const
    {
        return lights_;
    }

private:
    Camera camera_;
    std::deque<Entity> entities_;
    std::vector<Light> lights_;
};
