#pragma once

#ifdef RONG_PLATFROM_WINDOWS

extern Rongine::Application* Rongine::createApplication();

int main(int argc,char* argv[])
{
	auto app = Rongine::createApplication();
	app->run();
	delete app;
}

#endif