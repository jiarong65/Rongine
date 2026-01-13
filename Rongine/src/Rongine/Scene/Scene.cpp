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
        entity.AddComponent<TransformComponent>(); 
        static uint64_t s_NextID = 1;
        entity.AddComponent<IDComponent>(s_NextID++);
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

    Entity Scene::getEntityByUUID(uint64_t uuid)
    {
        auto view = m_registry.view<IDComponent>();
        for (auto entity : view)
        {
            const auto& idComp = view.get<IDComponent>(entity);
            if (idComp.ID == uuid)
                return { entity, this };
        }
        return {}; 
    }
}