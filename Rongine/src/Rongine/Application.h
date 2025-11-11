#pragma once

#include "Core.h"

namespace Rongine {
	class RONG_API  Application
	{
		public:
			Application();
			virtual ~Application();
			void run();
	};

	Application* createApplication();
}


