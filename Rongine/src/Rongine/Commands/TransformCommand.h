#pragma once
#include "Rongine/Commands/Command.h"	
#include "Rongine/Scene/Entity.h"
#include "Rongine/Scene/Components.h"

namespace Rongine {

	class TransformCommand : public Command
	{
	public:
		TransformCommand(Entity entity, const TransformComponent& oldTC, const TransformComponent& newTC)
			: m_Entity(entity), m_OldTC(oldTC), m_NewTC(newTC)
		{
		}

		virtual bool Execute() override;

		virtual void Undo() override;

		virtual std::string GetName() const override { return "Transform Entity"; }

		// 优化：如果在同一个实体上连续操作，直接更新终点，不产生新命令
		virtual bool MergeWith(Command* other) override;

	private:
		void UpdateComponent(const TransformComponent& tc);

	private:
		Entity m_Entity;
		TransformComponent m_OldTC;
		TransformComponent m_NewTC;
	};
}