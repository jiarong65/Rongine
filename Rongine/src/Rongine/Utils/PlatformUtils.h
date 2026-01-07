#pragma once
#include <string>

namespace Rongine {

	class FileDialogs
	{
	public:
		// 返回选择的文件路径，如果取消则返回空字符串
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};
}