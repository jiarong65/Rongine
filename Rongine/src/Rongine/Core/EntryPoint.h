#pragma once

#ifdef RONG_PLATFORM_WINDOWS

#include "Log.h"

extern Rongine::Application* Rongine::createApplication();

int main(int argc,char* argv[])
{
	Rongine::Log::init();
	RONG_CORE_INFO("  init sucess!  ");

	auto app = Rongine::createApplication();
	app->run();
	delete app;
}

#endif