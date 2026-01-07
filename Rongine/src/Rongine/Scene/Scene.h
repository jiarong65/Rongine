#pragma once
#include "entt.hpp"
#include "Rongine/Core/Timestep.h"

namespace Rongine {

    class Entity; // 前置声明

    class Scene
    {
    public:
        Scene();
        ~Scene();

        Entity createEntity(const std::string& name = std::string());
        void destroyEntity(Entity entity);

        void onUpdate(Timestep ts); // 暂时预留，以后处理物理或脚本

        // 让 Entity 类和 EditorLayer 能够访问注册表
        template<typename... Components>
        auto getAllEntitiesWith()
        {
            return m_registry.view<Components...>();
        }

    private:
        entt::registry m_registry;

        friend class Entity;
        friend class EditorLayer; // 让 EditorLayer 能直接操作 registry (渲染遍历用)
    };
}