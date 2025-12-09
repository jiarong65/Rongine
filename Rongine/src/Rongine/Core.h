#pragma once
#include <memory>

#ifdef RONG_PLATFORM_WINDOWS
#if RONG_DYNAMIC_LINK
	#ifdef RONG_BUILD_DLL
		#define RONG_API __declspec(dllexport)
	#else
		#define RONG_API __declspec(dllimport)
	#endif
#else
	#define RONG_API
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

namespace Rongine {

	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T>
	using Scope = std::unique_ptr<T>;
	
}
