#pragma once

#include "Core.h"
#include "Rongine/Events/Event.h"

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


