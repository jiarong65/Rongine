#pragma once
#include "Command.h"
#include "Rongine/Scene/Entity.h"
#include "Rongine/Scene/Components.h"

namespace Rongine {

	// 在内存中暂存被删除实体的数据
	struct EntityDataBackup
	{
		uint64_t UUID = 0;
		std::string Tag;
		TransformComponent Transform;
		bool HasCAD = false;
		CADGeometryComponent CAD; // 注意：CAD组件里有指针，需要特殊处理
		bool HasMesh = false;
		MeshComponent Mesh;       // Mesh组件里有VA，shared_ptr自动管理，可以直接拷贝
	};

	class DeleteCommand : public Command
	{
	public:
		DeleteCommand(Entity entity);
		virtual ~DeleteCommand(); // 需要处理 CAD 组件的内存

		virtual bool Execute() override;
		virtual void Undo() override;
		virtual std::string GetName() const override { return "Delete Entity"; }

	private:
		Scene* m_Scene;             // 也就是 m_ActiveScene
		entt::entity m_EntityHandle;// 实体的 ID (entt句柄)
		EntityDataBackup m_Backup;  // 数据备份
	};
}