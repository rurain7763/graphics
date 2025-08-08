#pragma once

#include "Core.h"

namespace flaw {
	class LogSink {
	public:
		virtual void Log(const char* message) = 0;
		virtual void Flush() = 0;
	};

	class FAPI Log {
	public:
		static void Initialize();
		static void Cleanup();

		static void PushLogSink(Ref<LogSink> sink);

		static void Info(const char* message, ...);
		static void Warn(const char* message, ...);
		static void Error(const char* message, ...);
		static void Fatal(const char* message, ...);
	};
}

#define LOG_INFO(msg, ...) flaw::Log::Info("%s/%s/%d: " msg, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(msg, ...) flaw::Log::Warn("%s/%s/%d: " msg, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) flaw::Log::Error("%s/%s/%d: " msg, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOG_FATAL(msg, ...) flaw::Log::Fatal("%s/%s/%d: " msg, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
