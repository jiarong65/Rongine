#include "Rongpch.h"
#include "CADModifyCommand.h"
#include "Rongine/Scene/Components.h"
#include "Rongine/CAD/CADMesher.h" 
#include <BRepBuilderAPI_Copy.hxx> 

namespace Rongine {

	CADModifyCommand::CADModifyCommand(Entity entity)
	{
		if (!entity) return;

		m_Scene = entity.getScene();

		// 保存 UUID
		if (entity.HasComponent<IDComponent>())
			m_EntityUUID = entity.GetComponent<IDComponent>().ID;

		// 备份当前的形状 (Undo 用的状态)
		if (entity && entity.HasComponent<CADGeometryComponent>())
		{
			auto& cad = entity.GetComponent<CADGeometryComponent>();
			if (cad.ShapeHandle)
			{
				m_OldShape = DeepCopyShape((TopoDS_Shape*)cad.ShapeHandle);
			}
		}
	}

	CADModifyCommand::~CADModifyCommand()
	{
		// 只有命令被销毁时（比如清空历史栈），才释放这些备份的 Shape
		if (m_OldShape) delete m_OldShape;
		if (m_NewShape) delete m_NewShape;
	}

	void CADModifyCommand::CaptureNewState()
	{
		Entity entity = GetEntity();
		// 备份修改后的形状 (Redo 用的状态)
		if (entity && entity.HasComponent<CADGeometryComponent>())
		{
			auto& cad = entity.GetComponent<CADGeometryComponent>();
			if (cad.ShapeHandle)
			{
				m_NewShape = DeepCopyShape((TopoDS_Shape*)cad.ShapeHandle);
			}
		}
	}

	bool CADModifyCommand::Execute()
	{
		// Redo: 应用新形状
		if (m_NewShape)
		{
			ApplyShape(m_NewShape);
			return true;
		}
		return false;
	}

	void CADModifyCommand::Undo()
	{
		// Undo: 恢复旧形状
		if (m_OldShape)
		{
			ApplyShape(m_OldShape);
		}
	}

	void CADModifyCommand::ApplyShape(TopoDS_Shape* sourceShape)
	{
		Entity entity = GetEntity(); // <--- 动态获取
		if (!entity) return;

		if (!entity.HasComponent<CADGeometryComponent>()) return;

		auto& cad = entity.GetComponent<CADGeometryComponent>();

		//  释放当前实体持有的 Shape 内存 (如果是 new 出来的)
		if (cad.ShapeHandle) delete (TopoDS_Shape*)cad.ShapeHandle; 

		// 将备份的数据 深拷贝 一份给实体
		cad.ShapeHandle = DeepCopyShape(sourceShape);

		// 网格重建
		CADMesher::RebuildMesh(entity);
	}

	TopoDS_Shape* CADModifyCommand::DeepCopyShape(const TopoDS_Shape* shape)
	{
		if (!shape) return nullptr;
		BRepBuilderAPI_Copy copier(*shape);
		// 返回一个新的堆内存对象
		return new TopoDS_Shape(copier.Shape());
	}

	Entity CADModifyCommand::GetEntity()
	{
		if (m_Scene && m_EntityUUID != 0)
			return m_Scene->getEntityByUUID(m_EntityUUID);
		return {};
	}
}