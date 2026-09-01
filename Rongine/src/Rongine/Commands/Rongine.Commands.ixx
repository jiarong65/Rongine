module;
#include <string>
#include <vector>
#include <cstddef>
#include <cstdio>

export module Rongine.Commands;

export namespace Rongine {

	class Command
	{
	public:
		virtual ~Command() = default;

		virtual bool Execute() = 0;
		virtual void Undo() = 0;
		virtual bool MergeWith(Command* other) { return false; }
		virtual std::string GetName() const = 0;
	};

	class CommandHistory
	{
	public:
		static void Push(Command* cmd);
		static void Undo();
		static void Redo();
		static void Clear();

	private:
		static std::vector<Command*> s_UndoStack;
		static std::vector<Command*> s_RedoStack;
		static const size_t s_MaxHistory = 100;
	};

	std::vector<Command*> CommandHistory::s_UndoStack;
	std::vector<Command*> CommandHistory::s_RedoStack;

	void CommandHistory::Push(Command* cmd)
	{
		if (cmd->Execute())
		{
			if (!s_UndoStack.empty())
			{
				if (s_UndoStack.back()->MergeWith(cmd))
				{
					delete cmd;
					return;
				}
			}

			s_UndoStack.push_back(cmd);

			for (auto* c : s_RedoStack) delete c;
			s_RedoStack.clear();

			if (s_UndoStack.size() > s_MaxHistory)
			{
				delete s_UndoStack.front();
				s_UndoStack.erase(s_UndoStack.begin());
			}
		}
		else
		{
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

		std::printf("Undo: %s\n", cmd->GetName().c_str());
	}

	void CommandHistory::Redo()
	{
		if (s_RedoStack.empty()) return;

		Command* cmd = s_RedoStack.back();
		s_RedoStack.pop_back();

		cmd->Execute();
		s_UndoStack.push_back(cmd);

		std::printf("Redo: %s\n", cmd->GetName().c_str());
	}

	void CommandHistory::Clear()
	{
		for (auto* c : s_UndoStack) delete c;
		s_UndoStack.clear();
		for (auto* c : s_RedoStack) delete c;
		s_RedoStack.clear();
	}
}
