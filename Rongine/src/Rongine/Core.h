#pragma once

#ifdef RONG_PLATFROM_WINDOWS
	#ifdef RONG_BUILD_DLL
		#define RONG_API __declspec(dllexport)
	#else
		#define RONG_API __declspec(dllimport)
	#endif
#else
	#error RONGINE ONLY SUPPORT WINDOWS 
#endif 
