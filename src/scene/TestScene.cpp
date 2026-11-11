#include "TestScene.hpp"
#include "assets/AssetLibrary.hpp"
#include "renderer/mesh/MeshPrimitives.hpp"


namespace
{
    Material createLanternMaterial(AssetLibrary& assets)
    {
        const std::string root = "resources/models/lantern/";
        Material material;
        material.baseColor = assets.texture(root + "Lantern_baseColor.png", true);
        material.normal = assets.texture(root + "Lantern_normal.png", false);
        material.metallicRoughness =
            assets.texture(root + "Lantern_roughnessMetallic.png", false);
        material.emissive = assets.texture(root + "Lantern_emissive.png", true);
        material.params.metallic = 1;
        material.params.roughness = 1;
        material.params.emissive = {2, 2, 2, 0};

        return material;
    }

    Material createPavingMaterial(AssetLibrary& assets)
    {
        const std::string root = "resources/textures/paving-stones/";
        Material material;
        material.baseColor = assets.texture(root + "Color.jpg", true);
        material.normal = assets.texture(root + "NormalGL.jpg", false);
        material.roughness = assets.texture(root + "Roughness.jpg", false);
        material.params.roughness = 1;
        material.params.normalStrength = 1;
        material.params.uvTransform = {6, 6, 0, 0};

        return material;
    }
}

std::unique_ptr<Scene> createTestScene(MTL::Device* device)
{
    auto scene = std::make_unique<Scene>();
    AssetLibrary assets(device);

    auto& lantern = scene->addEntity(
        "Lantern",
        assets.mesh("resources/models/lantern/lantern.obj"),
        createLanternMaterial(assets)
    );
    lantern.transform.rotation[1] = 0.35f;

    auto& ground = scene->addEntity(
        "Stone paving", MeshPrimitives::plane(device, 12), createPavingMaterial(assets)
    );
    ground.transform.position.y = -0.01f;

    const auto lampPosition =
        simd_mul(lantern.transform.modelMatrix(), simd_float4{0.673f, 2.0f, 0, 1});
    Light lamp;
    lamp.type = LightType::Point;
    lamp.position = lampPosition.xyz;
    lamp.color = {1.0f, 0.57f, 0.22f};
    lamp.intensity = 22.0f;
    lamp.range = 7.0f;
    lamp.showMarker = false;
    scene->addLight(lamp);

    scene->camera().position = {4, 3.1f, -6};
    scene->camera().target = {0, 1.35f, 0};
    scene->camera().fovY = 45;

    return scene;
}
