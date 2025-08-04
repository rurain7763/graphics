#include "pch.h"
#include "Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/details/log_msg.h>
#include <cstdarg>

namespace flaw {
	class SpdLogSink : public spdlog::sinks::sink {
	public:
		SpdLogSink(Ref<LogSink> logSink) : _logSink(logSink) {}

		void log(const spdlog::details::log_msg& msg) override {
			_logSink->Log(msg.payload.data());
		}

		void flush() override {
			_logSink->Flush();
		}

		void set_pattern(const std::string& pattern) override {
			set_formatter(std::make_unique<spdlog::pattern_formatter>(pattern));
		}

		void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {
			_formatter = std::move(sink_formatter);
		}
	
	private:
		Ref<LogSink> _logSink;
		Scope<spdlog::formatter> _formatter;
	};
	
	static std::shared_ptr<spdlog::logger> s_CoreLogger;
	static Ref<SpdLogSink> s_SpdLogSink;

	void Log::Initialize() {
		s_CoreLogger = spdlog::stdout_color_mt("FLAW");
		s_CoreLogger->set_level(spdlog::level::trace);

		spdlog::set_pattern("%^[%T] %n: %v%$");
	}

	void Log::Cleanup() {
		spdlog::shutdown();
	}

	void Log::PushLogSink(Ref<LogSink> sink) {
		s_SpdLogSink = CreateRef<SpdLogSink>(sink);
		s_CoreLogger->sinks().push_back(s_SpdLogSink);
	}

	void Log::Info(const char* message, ...) {
		char msg[1024];
		va_list args;
		va_start(args, message);
		vsprintf(msg, 1024, message, args);
		va_end(args);
		s_CoreLogger->info(msg);
	}

	void Log::Warn(const char* message, ...) {
		char msg[1024];
		va_list args;
		va_start(args, message);
		vsprintf(msg, 1024, message, args);
		va_end(args);
		s_CoreLogger->warn(msg);
	}

	void Log::Error(const char* message, ...) {
		char msg[1024];
		va_list args;
		va_start(args, message);
		vsprintf(msg, 1024, message, args);
		va_end(args);
		s_CoreLogger->error(msg);
	}

	void Log::Fatal(const char* message, ...) {
		char msg[1024];
		va_list args;
		va_start(args, message);
		vsprintf(msg, 1024, message, args);
		va_end(args);
		s_CoreLogger->critical(msg);
	}
}