#pragma once
#include "Command.h"
#include "Rongine/Scene/Entity.h"
#include "Rongine/Scene/SceneSerializer.h" // 用序列化来“克隆”实体数据

namespace Rongine {

    class DeleteCommand : public Command
    {
    public:
        DeleteCommand(Entity entity)
            //: m_Scene(entity.getContext())
        {
            // 1. 保存要删除的实体的所有数据 (利用序列化机制存成 YAML 节点，或者手动存组件)
            // 这里为了简单，我们用一种“伪删除”策略：
            // 实际上不 destroy，只是从 Scene 的 Registry 中移除？不，EnTT 的 destroy 是破坏性的。
            // 所以必须“克隆”一份数据。

            m_EntityID = (uint32_t)entity; // 保存 ID (如果有 UUID 更好)

            // 简单起见，我们暂存实体的关键组件数据
            // 注意：这需要你的 Component 都有拷贝构造函数（默认都有）
            // 在真正的引擎里，通常会序列化成内存流。
            // 这里我们用一个变通方法：只存 ID，但其实 Undo 时重新创建比较麻烦。

            // 【最简单的 Undo Delete 实现】：
            // 不要在 destroy 时真删除，而是把它移到一个“回收站”列表里（从 Scene 移除，但保留对象）。
            // 但 EnTT 不支持这样做。

            // 妥协方案：
            // 目前先不实现完美的“复活”，只实现“Gizmo 移动”的撤销。
            // 删除撤销比较复杂，涉及到 Entity ID 的重新分配。
            // 我们先把这个文件建好，留空，以后再填。
        }

        virtual bool Execute() override { return true; }
        virtual void Undo() override { }
        virtual std::string GetName() const override { return "Delete Entity"; }

    private:
        Scene* m_Scene;
        uint32_t m_EntityID;
    };
}