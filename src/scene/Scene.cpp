#include "Scene.hpp"

#include <stdexcept>
#include <utility>


Entity& Scene::addEntity(std::shared_ptr<Mesh> mesh)
{
    if (!mesh)
        throw std::invalid_argument("entity requires a mesh");

    Entity entity{};
    entity.mesh = std::move(mesh);

    entities_.push_back(std::move(entity));

    return entities_.back();
};


void Scene::update(float deltaTime)
{
    if (entities_.empty())
        return;
    entities_[0].transform.rotation[1] += deltaTime;
}