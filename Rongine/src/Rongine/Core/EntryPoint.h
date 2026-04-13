#pragma once

#ifdef RONG_PLATFORM_WINDOWS

#include "Log.h"

extern Rongine::Application* Rongine::createApplication();

// inline：若多个翻译单元包含本头，链接器会合并为单一入口（避免 LNK2005/LNK1169）
inline int main(int argc, char* argv[])
{
	Rongine::Log::init();
	RONG_CORE_INFO("  init sucess!  ");

	auto app = Rongine::createApplication();
	app->run();
	delete app;
}

#endif