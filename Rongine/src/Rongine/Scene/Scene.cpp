#include "Rongpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Components.h"

namespace Rongine {

    Scene::Scene() {}

    Scene::~Scene() {}

    Entity Scene::createEntity(const std::string& name)
    {
        Entity entity = { m_registry.create(), this };
        entity.AddComponent<TransformComponent>(); // 默认都有 Transform
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;
        return entity;
    }

    void Scene::destroyEntity(Entity entity)
    {
        m_registry.destroy(entity);
    }

    void Scene::onUpdate(Timestep ts)
    {
        // 这里暂时留空，以后可以在这里更新脚本组件
    }
}