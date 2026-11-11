#include "Scene.hpp"


#include <stdexcept>
#include <utility>


Entity&
Scene::addEntity(std::string name, std::shared_ptr<Mesh> mesh, Material material)
{
    if (!mesh)
    {
        throw std::invalid_argument("entity requires a mesh");
    }

    Entity entity{};
    entity.name = std::move(name);
    entity.mesh = std::move(mesh);
    entity.material = std::move(material);

    entities_.push_back(std::move(entity));

    return entities_.back();
}


void Scene::update(float)
{
}


Light& Scene::addLight(const Light& light)
{
    lights_.push_back(light);

    return lights_.back();
}
