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

            m_EntityID = (uint32_t)entity; // 保存 ID (如果有 UUID 更好)


        }

        virtual bool Execute() override { return true; }
        virtual void Undo() override { }
        virtual std::string GetName() const override { return "Delete Entity"; }

    private:
        Scene* m_Scene;
        uint32_t m_EntityID;
    };
}