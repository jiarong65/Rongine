#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include <memory>

namespace Rongine {
	class RONG_API Log
	{
	public:
		static void init();

		inline static std::shared_ptr<spdlog::logger>& getCoreLogger() { return s_CoreLogger; };
		inline static std::shared_ptr<spdlog::logger>& getClientLogger() { return s_ClientLogger; };
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#define RONG_CORE_TRACE(...)	::Rongine::Log::getCoreLogger()->trace(__VA_ARGS__)
#define RONG_CORE_INFO(...)		::Rongine::Log::getCoreLogger()->info(__VA_ARGS__)
#define RONG_CORE_WARN(...)		::Rongine::Log::getCoreLogger()->warn(__VA_ARGS__)
#define RONG_CORE_ERROR(...)	::Rongine::Log::getCoreLogger()->error(__VA_ARGS__)
#define RONG_CORE_FATAL(...)	::Rongine::Log::getCoreLogger()->fatal(__VA_ARGS__)

#define RONG_CLIENT_TRACE(...)	 ::Rongine::Log::getClientLogger()->trace(__VA_ARGS__)
#define RONG_CLIENT_INFO(...)	 ::Rongine::Log::getClientLogger()->info(__VA_ARGS__)
#define RONG_CLIENT_WARN(...)	 ::Rongine::Log::getClientLogger()->warn(__VA_ARGS__)
#define RONG_CLIENT_ERROR(...)	 ::Rongine::Log::getClientLogger()->error(__VA_ARGS__)
#define RONG_CLIENT_FATAL(...)   ::Rongine::Log::getClientLogger()->fatal(__VA_ARGS__)



