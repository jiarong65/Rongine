module;

#include <cstdint>
#include <string>
#include "Rongine/Core/Log.h"

export module Rongine.TransformCommand;

export import Rongine.Commands;
export import Rongine.Scene;
export import Rongine.SceneData;
export import Rongine.Core;

import Rongine.Components;

export namespace Rongine {

	class TransformCommand : public Command
	{
	public:
		TransformCommand(Entity entity, const TransformComponent& oldTC, const TransformComponent& newTC)
			: m_OldTC(oldTC), m_NewTC(newTC)
		{
			if (entity)
			{
				m_Scene = entity.getScene();

				if (entity.HasComponent<IDComponent>())
					m_EntityUUID = entity.GetComponent<IDComponent>().ID;
			}
		}

		virtual bool Execute() override;

		virtual void Undo() override;

		virtual std::string GetName() const override { return "Transform Entity"; }

		// 优化：如果在同一个实体上连续操作，直接更新终点，不产生新命令
		virtual bool MergeWith(Command* other) override;

	private:
		void UpdateComponent(const TransformComponent& tc);
		Entity GetEntity();

	private:
		Scene* m_Scene = nullptr;
		uint64_t m_EntityUUID = 0;

		TransformComponent m_OldTC;
		TransformComponent m_NewTC;
	};

	bool TransformCommand::Execute()
	{
		// 将实体设为“新状态”
		UpdateComponent(m_NewTC);
		return true;
	}
	void TransformCommand::Undo()
	{
		// 恢复到“旧状态”
		UpdateComponent(m_OldTC);
	}

	// 关键优化：如果在同一个实体上连续操作，直接更新终点，不产生新命令

	bool TransformCommand::MergeWith(Command* other)
	{
		//auto* nextCmd = dynamic_cast<TransformCommand*>(other);
		//if (!nextCmd) return false;

		//if (nextCmd->m_Entity == m_Entity)
		//{
		//	m_NewTC = nextCmd->m_NewTC; // 更新终点
		//	return true;
		//}
		return false;
	}

	void TransformCommand::UpdateComponent(const TransformComponent& tc)
	{
		Entity entity = GetEntity();

		if (entity && entity.HasComponent<TransformComponent>())
		{
			auto& comp = entity.GetComponent<TransformComponent>();
			comp.Translation = tc.Translation;
			comp.Rotation = tc.Rotation;
			comp.Scale = tc.Scale;
		}
	}

	Entity TransformCommand::GetEntity()
	{
		if (m_Scene && m_EntityUUID != 0)
			return m_Scene->getEntityByUUID(m_EntityUUID);
		return {};
	}
}
