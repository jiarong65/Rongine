module;
#include <cstdint>
#include <string>
#include <utility>
#include <cstdio>
#include "entt.hpp"

export module Rongine.Scene;

export import Rongine.Core;
export import Rongine.SceneData;

export namespace Rongine {

	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity createEntity(const std::string& name = std::string());
		void destroyEntity(Entity entity);

		void onUpdate(Timestep ts);

		template<typename... Components>
		auto getAllEntitiesWith()
		{
			return m_registry.view<Components...>();
		}

		entt::registry& getRegistry() { return m_registry; }

		Entity getEntityByUUID(uint64_t uuid);

	private:
		entt::registry m_registry;

		friend class Entity;
	};

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
				std::printf("Entity already has component!\n");
				return GetComponent<T>();
			}

			return m_scene->m_registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
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
			return m_scene->m_registry.try_get<T>(m_EntityHandle) != nullptr;
		}

		template<typename T>
		void RemoveComponent()
		{
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

	inline Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_scene(scene) {
	}

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
		entity.AddComponent<MaterialComponent>();
		return entity;
	}

	void Scene::destroyEntity(Entity entity)
	{
		m_registry.destroy(entity);
	}

	void Scene::onUpdate(Timestep ts)
	{
		// reserved for future physics/script updates
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
