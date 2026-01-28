#pragma once
#include "Scene.h"
#include "Rongine/Core/Log.h"

namespace Rongine {

    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene);
        Entity(const Entity& other) = default;

        Scene* getScene() { return m_scene; }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            if (HasComponent<T>())
            {
                RONG_CORE_WARN("Entity already has component!");
                return GetComponent<T>();
            }
            
            return m_scene->m_registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent()
        {
            RONG_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            return m_scene->m_registry.get<T>(m_EntityHandle);
        }

        template<typename T, typename... Args>
        T& GetOrAddComponent(Args&&... args)
        {
            if (HasComponent<T>())
                return GetComponent<T>();
            return m_scene->m_registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template<typename T>
        bool HasComponent()
        {
            // try_get 返回指针，如果组件存在则非空，不存在则为 nullptr
            // 这在几乎所有 EnTT 版本中都通用
            return m_scene->m_registry.try_get<T>(m_EntityHandle) != nullptr;
        }

        template<typename T>
        void RemoveComponent()
        {
            RONG_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            // 注意：m_Scene 大小写修正为 m_scene
            m_scene->m_registry.remove<T>(m_EntityHandle);
        }

        operator bool() const { return m_EntityHandle != entt::null; }

        operator uint32_t() const { return (uint32_t)m_EntityHandle; }
        operator entt::entity() const { return m_EntityHandle; }

        bool operator==(const Entity& other) const
        {
            return m_EntityHandle == other.m_EntityHandle && m_scene == other.m_scene;
        }

        bool operator!=(const Entity& other) const { return !(*this == other); }

    private:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_scene = nullptr;
    };

    // 内联实现构造函数，避免链接错误
    inline Entity::Entity(entt::entity handle, Scene* scene)
        : m_EntityHandle(handle), m_scene(scene) {
    }
}