#pragma once

#include <boost/stacktrace.hpp>
#include <dia2.h>
#include <diacreate.h>
#include <fmt/format.h>
#include <frozen/map.h>
#include <infoware/cpu.hpp>
#include <infoware/gpu.hpp>
#include <infoware/system.hpp>

#undef cdecl  // Workaround for Clang 14 CMake configure error.

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#undef GetObject  // Have to do this because PCH pulls in spdlog->winbase.h->windows.h->wingdi.h, which redfines GetObject


// Compatible declarations with other sample projects.
#define DLLEXPORT __declspec(dllexport)

using namespace std::literals;

#include <Windows.h>
#include <optional>
#include <stringapiset.h>
#include <filesystem>
#include <locale>
#include <string_view>
#include <xstring>
#include <stdlib.h>
#include <source_location>

#define SKSE_MAKE_SOURCE_LOGGER(a_func, a_type)                                       \
                                                                                      \
	template <class... Args>                                                          \
	struct [[maybe_unused]] a_func                                                    \
	{                                                                                 \
		a_func() = delete;                                                            \
                                                                                      \
		explicit a_func(                                                              \
			fmt::format_string<Args...> a_fmt,                                        \
			Args&&... a_args,                                                         \
			std::source_location a_loc = std::source_location::current()) \
		{                                                                             \
			spdlog::log(                                                              \
				spdlog::source_loc{                                                   \
					a_loc.file_name(),                                                \
					static_cast<int>(a_loc.line()),                                   \
					a_loc.function_name() },                                          \
				spdlog::level::a_type,                                                \
				a_fmt,                                                                \
				std::forward<Args>(a_args)...);                                       \
		}                                                                             \
	};                                                                                \
                                                                                      \
	template <class... Args>                                                          \
	a_func(fmt::format_string<Args...>, Args&&...) -> a_func<Args...>;

namespace logger
{
	SKSE_MAKE_SOURCE_LOGGER(trace, trace);
	SKSE_MAKE_SOURCE_LOGGER(debug, debug);
	SKSE_MAKE_SOURCE_LOGGER(info, info);
	SKSE_MAKE_SOURCE_LOGGER(warn, warn);
	SKSE_MAKE_SOURCE_LOGGER(error, err);
	SKSE_MAKE_SOURCE_LOGGER(critical, critical);

	[[nodiscard]] std::optional<std::filesystem::path> log_directory();
}

extern "C" IMAGE_DOS_HEADER __ImageBase;
#include <regex>
#include <cstdlib>

namespace util
{
#undef GetModuleFileName

	std::uint32_t GetModuleFileName(
		void* a_module,
		char* a_filename,
		std::uint32_t a_size) noexcept;

	std::uint32_t GetModuleFileName(
		void* a_module,
		wchar_t* a_filename,
		std::uint32_t a_size) noexcept;

	void TerminateProcess(
		void* a_process,
		uint32_t a_exitCode) noexcept;

	[[nodiscard]] inline auto utf8_to_utf16(std::string_view a_in) noexcept
		-> std::optional<std::wstring>
	{
		const auto len = mbstowcs(nullptr, a_in.data(), a_in.length());
		if (len == 0) {
			return std::nullopt;
		}

		std::wstring out(len, '\0');
		mbstowcs(out.data(), a_in.data(), a_in.length());

		return out;
	}

	[[nodiscard]] inline auto utf16_to_utf8(std::wstring_view a_in) noexcept
		-> std::optional<std::string>
	{
		const wchar_t* input = a_in.data();

		// Count required buffer size (plus one for null-terminator).
		size_t size = (wcslen(input) + 1) * sizeof(wchar_t);
		std::string out(size, '\0');

		size_t convertedSize;
		wcstombs_s(&convertedSize, out.data(), size, input, size);

		if (strcmp(out.c_str(), ""))
			return std::nullopt;

		return out;
	}

	template <class T, class U>
	[[nodiscard]] auto adjust_pointer(U* a_ptr, std::ptrdiff_t a_adjust) noexcept
	{
		auto addr = a_ptr ? reinterpret_cast<std::uintptr_t>(a_ptr) + a_adjust : 0;
		if constexpr (std::is_const_v<U> && std::is_volatile_v<U>) {
			return reinterpret_cast<std::add_cv_t<T>*>(addr);
		} else if constexpr (std::is_const_v<U>) {
			return reinterpret_cast<std::add_const_t<T>*>(addr);
		} else if constexpr (std::is_volatile_v<U>) {
			return reinterpret_cast<std::add_volatile_t<T>*>(addr);
		} else {
			return reinterpret_cast<T*>(addr);
		}
	}

	inline bool report_and_error(std::string_view a_msg, bool a_fail = true,
		std::source_location a_loc = std::source_location::current())
	{
		const auto body = [&]() -> std::wstring {
			const std::filesystem::path p = a_loc.file_name();
			auto filename = p.lexically_normal().generic_string();

			const std::regex r{ R"((?:^|[\\\/])(?:include|src)[\\\/](.*)$)" };
			std::smatch matches;
			if (std::regex_search(filename, matches, r)) {
				filename = matches[1].str();
			}

			return utf8_to_utf16(
				std::format(
					"{}({}): {}"sv,
					filename,
					a_loc.line(),
					a_msg))
			    .value_or(L"<character encoding error>"s);
		}();

		const auto caption = []() {
			const auto maxPath = static_cast<std::size_t>(MAX_PATH);
			std::vector<wchar_t> buf;
			buf.reserve(maxPath);
			buf.resize(maxPath / 2);
			std::uint32_t result = 0;
			do {
				buf.resize(buf.size() * 2);
				result = GetModuleFileName(
					static_cast<void*>(std::addressof(__ImageBase)),
					buf.data(),
					static_cast<std::uint32_t>(buf.size()));
			} while (result && result == buf.size() && buf.size() <= (std::numeric_limits<std::uint32_t>::max)());

			if (result && result != buf.size()) {
				std::filesystem::path p(buf.begin(), buf.begin() + result);
				return p.filename().native();
			} else {
				return L""s;
			}
		}();

		spdlog::log(
			spdlog::source_loc{
				a_loc.file_name(),
				static_cast<int>(a_loc.line()),
				a_loc.function_name() },
			spdlog::level::critical,
			a_msg);

		if (a_fail) {
//#ifdef ENABLE_COMMONLIBSSE_TESTING
//			throw std::runtime_error(utf16_to_utf8(caption.empty() ? body.c_str() : caption.c_str())->c_str());
//#else
			MessageBox(nullptr, body.c_str(), (caption.empty() ? nullptr : caption.c_str()), 0);
			TerminateProcess(static_cast<void*>(::GetCurrentProcess()), EXIT_FAILURE);
//#endif
		}
		//return true;
		exit(1);
	}

	//[[noreturn]]
	 inline void report_and_fail(std::string_view a_msg,
		std::source_location a_loc = std::source_location::current())
	{
		report_and_error(a_msg, true, a_loc);
	}

	/* [[nodiscard]] inline auto module_name()
		-> std::string
	{
		const auto filename = REL::Module::get().filename();
		return util::utf16_to_utf8(filename)
		    .value_or("<unknown module name>"s);
	}*/
}

namespace stl
{
	template <
		class Enum,
		class Underlying = std::underlying_type_t<Enum>>
	class enumeration
	{
	public:
		using enum_type = Enum;
		using underlying_type = Underlying;

		static_assert(std::is_enum_v<enum_type>, "enum_type must be an enum");
		static_assert(std::is_integral_v<underlying_type>, "underlying_type must be an integral");

		constexpr enumeration() noexcept = default;

		constexpr enumeration(const enumeration&) noexcept = default;

		constexpr enumeration(enumeration&&) noexcept = default;

		template <class U2>  // NOLINTNEXTLINE(google-explicit-constructor)
		constexpr enumeration(enumeration<Enum, U2> a_rhs) noexcept :
			_impl(static_cast<underlying_type>(a_rhs.get()))
		{}

		template <class... Args>
		constexpr enumeration(Args... a_values) noexcept  //
			requires(std::same_as<Args, enum_type> && ...)
			:
			_impl((static_cast<underlying_type>(a_values) | ...))
		{}

		~enumeration() noexcept = default;

		constexpr enumeration& operator=(const enumeration&) noexcept = default;
		constexpr enumeration& operator=(enumeration&&) noexcept = default;

		template <class U2>
		constexpr enumeration& operator=(enumeration<Enum, U2> a_rhs) noexcept
		{
			_impl = static_cast<underlying_type>(a_rhs.get());
		}

		constexpr enumeration& operator=(enum_type a_value) noexcept
		{
			_impl = static_cast<underlying_type>(a_value);
			return *this;
		}

		[[nodiscard]] explicit constexpr operator bool() const noexcept { return _impl != static_cast<underlying_type>(0); }

		[[nodiscard]] constexpr enum_type operator*() const noexcept { return get(); }
		[[nodiscard]] constexpr enum_type get() const noexcept { return static_cast<enum_type>(_impl); }
		[[nodiscard]] constexpr underlying_type underlying() const noexcept { return _impl; }

		template <class... Args>
		constexpr enumeration& set(Args... a_args) noexcept  //
			requires(std::same_as<Args, enum_type> && ...)
		{
			_impl |= (static_cast<underlying_type>(a_args) | ...);
			return *this;
		}

		template <class... Args>
		constexpr enumeration& reset(Args... a_args) noexcept  //
			requires(std::same_as<Args, enum_type> && ...)
		{
			_impl &= ~(static_cast<underlying_type>(a_args) | ...);
			return *this;
		}

		template <class... Args>
		[[nodiscard]] constexpr bool any(Args... a_args) const noexcept  //
			requires(std::same_as<Args, enum_type> && ...)
		{
			return (_impl & (static_cast<underlying_type>(a_args) | ...)) != static_cast<underlying_type>(0);
		}

		template <class... Args>
		[[nodiscard]] constexpr bool all(Args... a_args) const noexcept  //
			requires(std::same_as<Args, enum_type> && ...)
		{
			return (_impl & (static_cast<underlying_type>(a_args) | ...)) == (static_cast<underlying_type>(a_args) | ...);
		}

		template <class... Args>
		[[nodiscard]] constexpr bool none(Args... a_args) const noexcept  //
			requires(std::same_as<Args, enum_type> && ...)
		{
			return (_impl & (static_cast<underlying_type>(a_args) | ...)) == static_cast<underlying_type>(0);
		}

	private:
		underlying_type _impl{ 0 };
	};

	template <class... Args>
	enumeration(Args...) -> enumeration<
		std::common_type_t<Args...>,
		std::underlying_type_t<
			std::common_type_t<Args...>>>;
}
