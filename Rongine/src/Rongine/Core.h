#pragma once

#ifdef RONG_PLATFORM_WINDOWS
	#ifdef RONG_BUILD_DLL
		#define RONG_API __declspec(dllexport)
	#else
		#define RONG_API __declspec(dllimport)
	#endif
#else
	#error RONGINE ONLY SUPPORT WINDOWS 
#endif 

#ifdef RONG_DEBUG
	#define RONG_ENABLE_ASSERTS
#endif 

#ifdef RONG_ENABLE_ASSERTS
	#define RONG_ASSERT(x, ...) { if(!(x)) { RONG_CLIENT_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define RONG_CORE_ASSERT(x, ...) { if(!(x)) { RONG_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define RONG_CLIENT_ASSERT(x, ...)
	#define RONG_CORE_ASSERT(x, ...)
#endif


#define BIT(x) (1<<x)

#define RONG_BIND_EVENT_FN(fn) [this](auto&&... args) {return this->fn(std::forward<decltype(args)>(args)...);}
