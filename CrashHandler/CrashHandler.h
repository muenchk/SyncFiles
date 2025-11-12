#pragma once

struct _EXCEPTION_RECORD;

#include <spdlog/logger.h>
#include <boost/stacktrace.hpp>
#include <span>
#include <memory>
#include <string_view>

namespace Crash
{
	namespace Modules
	{
		class Module;
	}

	class Debug
	{
	public:
		spdlog::level::level_enum logLevel{ spdlog::level::level_enum::info };
		spdlog::level::level_enum flushLevel{ spdlog::level::level_enum::trace };
		bool waitForDebugger{ false };
		std::string symcache{ "" };
		std::string crashDirectory{ "" };

		[[nodiscard]] static Debug* GetSingleton();
	};

	class Callstack
	{
	public:
		Callstack(const ::_EXCEPTION_RECORD& a_except);

		Callstack(boost::stacktrace::stacktrace a_stacktrace) :
			_stacktrace{ std::move(a_stacktrace) },
			_frames{ _stacktrace.begin(), _stacktrace.end() }
		{}

		void print(
			spdlog::logger& a_log,
			std::span<const std::unique_ptr<Modules::Module>> a_modules) const;

	private:
		[[nodiscard]] static std::string get_size_string(std::size_t a_size);

		[[nodiscard]] std::string get_format(std::size_t a_nameWidth) const;

		void print_probable_callstack(
			spdlog::logger& a_log,
			std::span<const std::unique_ptr<Modules::Module>> a_modules) const;

		void print_raw_callstack(spdlog::logger& a_log) const;

		boost::stacktrace::stacktrace _stacktrace;
		std::span<const boost::stacktrace::frame> _frames;
	};

	void Install(std::string a_crashPath);
	static std::string crashPath;
}
