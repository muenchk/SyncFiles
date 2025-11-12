#include "PCH.h"

namespace logger
{
	[[nodiscard]] std::optional<std::filesystem::path> log_directory()
	{
		return std::make_optional<std::filesystem::path>(".");
	}
}


namespace util
{
	std::uint32_t GetModuleFileName(
		void* a_module,
		char* a_filename,
		std::uint32_t a_size) noexcept
	{
		return static_cast<std::uint32_t>(
			::GetModuleFileNameA(
				static_cast<::HMODULE>(a_module),
				static_cast<::LPSTR>(a_filename),
				static_cast<::DWORD>(a_size)));
	}

	std::uint32_t GetModuleFileName(
		void* a_module,
		wchar_t* a_filename,
		std::uint32_t a_size) noexcept
	{
		return static_cast<std::uint32_t>(
			::GetModuleFileNameW(
				static_cast<::HMODULE>(a_module),
				static_cast<::LPWSTR>(a_filename),
				static_cast<::DWORD>(a_size)));
	}

	void TerminateProcess(
		void* a_process,
		uint32_t a_exitCode) noexcept
	{
		::TerminateProcess(
			static_cast<::HANDLE>(a_process),
			static_cast<::UINT>(a_exitCode));
#if defined(__clang__) || defined(__GNUC__)
		__builtin_unreachable();
#elif defined(_MSC_VER)
		__assume(false);
#endif
	}

}
