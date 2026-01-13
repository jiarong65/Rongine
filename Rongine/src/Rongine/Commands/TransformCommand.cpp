#include "Rongpch.h"
#include "Rongine/Commands/TransformCommand.h"
#include "Rongine/Core/Log.h"

namespace Rongine {
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