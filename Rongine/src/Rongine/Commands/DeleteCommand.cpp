#include "Rongpch.h"
#include "DeleteCommand.h"
#include "Rongine/Scene/Scene.h"
#include "Rongine/CAD/CADMesher.h"
#include <TopoDS_Shape.hxx>
#include <BRepBuilderAPI_Copy.hxx>

namespace Rongine {

	DeleteCommand::DeleteCommand(Entity entity)
		: m_Scene(entity.getScene()), m_EntityHandle(entity)
	{
        // 备份 ID
        if (entity.HasComponent<IDComponent>())
            m_Backup.UUID = entity.GetComponent<IDComponent>().ID;
		// 1. 备份 Tag
		if (entity.HasComponent<TagComponent>())
			m_Backup.Tag = entity.GetComponent<TagComponent>().Tag;

		// 2. 备份 Transform
		if (entity.HasComponent<TransformComponent>())
			m_Backup.Transform = entity.GetComponent<TransformComponent>();

		// 3. 备份 CAD (需要深拷贝 Shape)
		if (entity.HasComponent<CADGeometryComponent>())
		{
			m_Backup.HasCAD = true;
			m_Backup.CAD = entity.GetComponent<CADGeometryComponent>(); // 浅拷贝参数

			// 深拷贝 ShapeHandle，防止删除实体后指针失效
			if (m_Backup.CAD.ShapeHandle)
			{
				BRepBuilderAPI_Copy copier(*(TopoDS_Shape*)m_Backup.CAD.ShapeHandle);
				m_Backup.CAD.ShapeHandle = new TopoDS_Shape(copier.Shape());
			}
		}

		// 4. 备份 Mesh (shared_ptr 会自动增加引用计数，不会丢失)
		if (entity.HasComponent<MeshComponent>())
		{
			m_Backup.HasMesh = true;
			m_Backup.Mesh = entity.GetComponent<MeshComponent>();
			// 注意：MeshComponent 里的 map<int, TopoDS_Edge> 也会被拷贝，这很好
		}
	}

	DeleteCommand::~DeleteCommand()
	{
		// 如果这个命令被销毁了，说明要么历史被清空，要么不需要 Undo 了
		if (m_Backup.HasCAD && m_Backup.CAD.ShapeHandle)
		{
			delete (TopoDS_Shape*)m_Backup.CAD.ShapeHandle;
			m_Backup.CAD.ShapeHandle = nullptr;
		}
	}

    bool DeleteCommand::Execute()
    {
        // 确保句柄有效
        if (m_EntityHandle == entt::null) return false;

        Entity entity{ m_EntityHandle, m_Scene };

        // 检查实体是否在注册表中有效 (避免重复删除导致崩溃)
        if (m_Scene->getRegistry().valid(m_EntityHandle))
        {
            m_Scene->destroyEntity(entity);
            return true; 
        }
        return false;
    }

    void DeleteCommand::Undo()
    {
        // 1. 创建新实体 (这会生成一个新的 EntityID)
        Entity entity = m_Scene->createEntity(m_Backup.Tag);

        RONG_CORE_INFO("Undo Delete: Restoring Entity '{0}' (New ID: {1})", m_Backup.Tag, (uint32_t)entity);

        //恢复uuid
        if (entity.HasComponent<IDComponent>())
        {
            entity.GetComponent<IDComponent>().ID = m_Backup.UUID;
        }
        else
        {
            entity.AddComponent<IDComponent>(m_Backup.UUID);
        }

        // 2. 恢复 Transform
        if (entity.HasComponent<TransformComponent>())
            entity.GetComponent<TransformComponent>() = m_Backup.Transform;

        // 3. 恢复 CAD 组件
        bool restoredCAD = false;
        if (m_Backup.HasCAD)
        {
            auto& cad = entity.AddComponent<CADGeometryComponent>();
            cad = m_Backup.CAD; // 拷贝参数 (Width/Height等)

            // 深拷贝 Shape
            if (m_Backup.CAD.ShapeHandle)
            {
                BRepBuilderAPI_Copy copier(*(TopoDS_Shape*)m_Backup.CAD.ShapeHandle);
                cad.ShapeHandle = new TopoDS_Shape(copier.Shape());
                entity.AddComponent<MeshComponent>();

                // 因为 Shape 是新生成的，不能直接拷贝旧的 MeshComponent,旧的内存已经释放了。
                // 必须调用 RebuildMesh 重新生成网格和映射表
                CADMesher::RebuildMesh(entity);
                restoredCAD = true;
            }
        }

        // 4. 恢复 Mesh 组件 (仅当没有 CAD 组件时才手动恢复)
        if (!restoredCAD && m_Backup.HasMesh)
        {
            auto& mesh = entity.AddComponent<MeshComponent>();
            mesh = m_Backup.Mesh;
        }

        // 5. 更新句柄
        // 因为 Undo 创建了一个新实体，ID 变了。
        m_EntityHandle = (entt::entity)entity;
    }
}