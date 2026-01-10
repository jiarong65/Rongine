#include "Rongpch.h"
#include "Command.h"
#include "Rongine/Core/Log.h"

namespace Rongine {

	std::vector<Command*> CommandHistory::s_UndoStack;
	std::vector<Command*> CommandHistory::s_RedoStack;

	void CommandHistory::Push(Command* cmd)
	{
		// 1. 尝试执行
		if (cmd->Execute())
		{
			// 2. 尝试合并 (如果栈顶命令和新命令类型一致且支持合并)
			if (!s_UndoStack.empty())
			{
				if (s_UndoStack.back()->MergeWith(cmd))
				{
					// 合并成功，删除新命令对象 (因为它已经融合进上一个了)
					delete cmd;
					return;
				}
			}

			// 3. 压入撤销栈
			s_UndoStack.push_back(cmd);

			// 4. 清空重做栈 (产生新分支，旧的重做路径失效)
			for (auto* c : s_RedoStack) delete c;
			s_RedoStack.clear();

			// 5. 限制历史长度
			if (s_UndoStack.size() > s_MaxHistory)
			{
				delete s_UndoStack.front();
				s_UndoStack.erase(s_UndoStack.begin());
			}
		}
		else
		{
			// 执行失败，直接删除
			delete cmd;
		}
	}

	void CommandHistory::Undo()
	{
		if (s_UndoStack.empty()) return;

		Command* cmd = s_UndoStack.back();
		s_UndoStack.pop_back();

		cmd->Undo();
		s_RedoStack.push_back(cmd);

		RONG_CORE_INFO("Undo: {0}", cmd->GetName());
	}

	void CommandHistory::Redo()
	{
		if (s_RedoStack.empty()) return;

		Command* cmd = s_RedoStack.back();
		s_RedoStack.pop_back();

		cmd->Execute();
		s_UndoStack.push_back(cmd);

		RONG_CORE_INFO("Redo: {0}", cmd->GetName());
	}

	void CommandHistory::Clear()
	{
		for (auto* c : s_UndoStack) delete c;
		s_UndoStack.clear();
		for (auto* c : s_RedoStack) delete c;
		s_RedoStack.clear();
	}
}