#pragma once
#include <string>
#include <vector>
#include <stack>
#include "Rongine/Core/Core.h"

namespace Rongine {

	// --- 抽象命令基类 ---
	class Command
	{
	public:
		virtual ~Command() = default;

		// 执行命令 (Do / Redo)
		// 返回 true 表示执行成功
		virtual bool Execute() = 0;

		// 撤销命令 (Undo)
		virtual void Undo() = 0;

		// 合并命令 (用于 Gizmo 拖拽优化，避免每一帧都存一个命令)
		// 如果返回 true，说明合并成功，旧命令会被丢弃
		virtual bool MergeWith(Command* other) { return false; }

		// 获取命令名称 (用于 UI 显示，例如 "Undo Move")
		virtual std::string GetName() const = 0;
	};

	// --- 命令历史管理器 (全局静态) ---
	class CommandHistory
	{
	public:
		// 执行并压入新命令
		static void Push(Command* cmd);

		// 撤销 (Ctrl+Z)
		static void Undo();

		// 重做 (Ctrl+Y)
		static void Redo();

		// 清空历史 (比如加载新场景时)
		static void Clear();

	private:
		static std::vector<Command*> s_UndoStack;
		static std::vector<Command*> s_RedoStack;
		static const size_t s_MaxHistory = 100; // 限制步数
	};

}