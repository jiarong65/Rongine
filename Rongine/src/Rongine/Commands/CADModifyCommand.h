#pragma once
#include "Command.h"
#include "Rongine/Scene/Entity.h"
#include <TopoDS_Shape.hxx>

namespace Rongine {

	class CADModifyCommand : public Command
	{
	public:
		// 构造函数：在修改发生之前调用，自动捕获当前形状作为 OldShape
		CADModifyCommand(Entity entity);

		virtual ~CADModifyCommand();

		// 在修改发生之后调用，捕获新形状
		void CaptureNewState();

		virtual bool Execute() override; // Redo 
		virtual void Undo() override;    // Undo 

		virtual std::string GetName() const override { return "Modify CAD Geometry"; }

	private:
		// 辅助函数：应用形状并重建网格
		void ApplyShape(TopoDS_Shape* sourceShape);
		// 辅助函数：深拷贝 Shape
		TopoDS_Shape* DeepCopyShape(const TopoDS_Shape* shape);

		// 辅助函数：获取当前有效的实体
		Entity GetEntity();

	private:
		Scene* m_Scene = nullptr;
		uint64_t m_EntityUUID = 0;
		TopoDS_Shape* m_OldShape = nullptr; // 堆内存中的备份
		TopoDS_Shape* m_NewShape = nullptr; // 堆内存中的备份
	};
}