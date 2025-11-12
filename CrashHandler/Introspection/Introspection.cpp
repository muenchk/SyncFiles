#include "Introspection/Introspection.h"

#include "Modules/ModuleHandler.h"
#include "PDB/PdbHandler.h"
#define MAGIC_ENUM_RANGE_MAX 256
#include <magic_enum/magic_enum.hpp>
#include <DbgHelp.h>
#include <fmt/format.h>
#include <fmt/core.h>
#include "PCH.h"

#include <variant>
#include <algorithm>
#include <execution>

namespace Crash::Introspection
{
	[[nodiscard]] const Modules::Module* get_module_for_pointer(
		const void* a_ptr,
		std::span<const module_pointer> a_modules) noexcept
	{
		const auto it = std::lower_bound(
			a_modules.rbegin(),
			a_modules.rend(),
			reinterpret_cast<std::uintptr_t>(a_ptr),
			[](auto&& a_lhs, auto&& a_rhs) noexcept {
				return a_lhs->address() >= a_rhs;
			});
		return it != a_modules.rend() && (*it)->in_range(a_ptr) ? it->get() : nullptr;
	}

	namespace detail
	{
		class Integer
		{
		public:
			Integer(std::size_t a_value) noexcept :
				_value(a_value),
				name_string(a_value >> 63 ?
								fmt::format(fmt::runtime("(size_t) [uint: {} int: {}]"s), _value, static_cast<std::make_signed_t<size_t>>(_value)) :
								fmt::format(fmt::runtime("(size_t) [{}]"s), _value))
			{
			}

			[[nodiscard]] std::string name() const { return name_string; }

		private:
			const std::size_t _value;
			const std::string name_string;
		};

		class Pointer
		{
		public:
			Pointer() noexcept = default;

			Pointer(const void* a_ptr, std::span<const module_pointer> a_modules) noexcept :
				_module(get_module_for_pointer(a_ptr, a_modules))
			{
				if (_module) {
					_ptr = a_ptr;
				}
			}

			[[nodiscard]] std::string name() const
			{
				if (_module) {
					const auto address = reinterpret_cast<std::uintptr_t>(_ptr);
					const auto pdbDetails = Crash::PDB::pdb_details(_module->path(), address - _module->address());
					const auto assembly = _module->assembly((const void*)address);
					if (!pdbDetails.empty())
						return fmt::format(
							"(void* -> {}+{:07X}\t{} | {})"sv,
							_module->name(),
							address - _module->address(),
							assembly,
							pdbDetails);
					return fmt::format(
						"(void* -> {}+{:07X}\t{})"sv,
						_module->name(),
						address - _module->address(),
						assembly);
				} else {
					return "(void*)"s;
				}
			}

		private:
			const Modules::Module* _module{ nullptr };
			const void* _ptr{ nullptr };
		};

		class Polymorphic
		{
		public:
			explicit Polymorphic(std::string_view a_mangled) noexcept :
				_mangled{ a_mangled }
			{
				// NOLINTNEXTLINE(readability-simplify-subscript-expr)
				assert(_mangled.size() > 1 && _mangled.data()[_mangled.size()] == '\0');
			}

			[[nodiscard]] std::string name() const
			{
				const auto demangle = [](const char* a_in, char* a_out, std::uint32_t a_size) {
					static std::mutex m;
					std::lock_guard l{ m };
					return UnDecorateSymbolName(
						a_in,
						a_out,
						a_size,
						(UNDNAME_NO_MS_KEYWORDS) |
							(UNDNAME_NO_FUNCTION_RETURNS) |
							(UNDNAME_NO_ALLOCATION_MODEL) |
							(UNDNAME_NO_ALLOCATION_LANGUAGE) |
							(UNDNAME_NO_THISTYPE) |
							(UNDNAME_NO_ACCESS_SPECIFIERS) |
							(UNDNAME_NO_THROW_SIGNATURES) |
							(UNDNAME_NO_RETURN_UDT_MODEL) |
							(UNDNAME_NAME_ONLY) |
							(UNDNAME_NO_ARGUMENTS) |
							static_cast<std::uint32_t>(0x8000));  // Disable enum/class/struct/union prefix
				};

				std::array<char, 0x1000> buf{ '\0' };
				const auto len = demangle(
					_mangled.data() + 1,
					buf.data(),
					static_cast<std::uint32_t>(buf.size()));

				if (len != 0) {
					return fmt::format(
						"({}*)"sv,
						std::string_view{ buf.data(), len });
				} else {
					return "(ERROR)"s;
				}
			}

		private:
			std::string_view _mangled;
		};

		class String
		{
		public:
			String(std::string_view a_str) noexcept :
				_str(a_str)
			{}

			[[nodiscard]] std::string name() const
			{
				return fmt::format("(char*) \"{}\""sv, _str);
			}

		private:
			std::string_view _str;
		};

		using analysis_result = std::variant<
			Integer,
			Pointer,
			Polymorphic,
			String>;

		template <class T, class... Args>
		[[nodiscard]] analysis_result make_result(Args&&... a_args) noexcept(
			std::is_nothrow_constructible_v<T, Args...>)
		{
			return analysis_result(std::in_place_type_t<T>{}, std::forward<Args>(a_args)...);
		}

		[[nodiscard]] auto analyze_polymorphic(
			void* a_ptr,
			std::span<const module_pointer> a_modules) noexcept
			-> std::optional<analysis_result>
		{
			try {
				const auto vtable = *reinterpret_cast<void**>(a_ptr);
				const auto mod = get_module_for_pointer(vtable, a_modules);
				if (!mod || !mod->in_rdata_range(vtable)) {
					return std::nullopt;
				}

				const auto col =
					*reinterpret_cast<RTTI::CompleteObjectLocator**>(
						reinterpret_cast<std::size_t*>(vtable) - 1);
				if (mod != get_module_for_pointer(col, a_modules) || !mod->in_rdata_range(col)) {
					return std::nullopt;
				}

				const auto typeDesc =
					reinterpret_cast<RTTI::TypeDescriptor*>(
						mod->address() + col->typeDescriptor.offset());
				if (mod != get_module_for_pointer(typeDesc, a_modules) || !mod->in_data_range(typeDesc)) {
					return std::nullopt;
				}

				if (*reinterpret_cast<const void**>(typeDesc) != mod->type_info()) {
					return std::nullopt;
				}

				return make_result<Polymorphic>(typeDesc->mangled_name());
			} catch (...) {
				return std::nullopt;
			}
		}

		[[nodiscard]] auto analyze_string(void* a_ptr) noexcept
			-> std::optional<analysis_result>
		{
			try {
				const auto printable = [](char a_ch) noexcept {
					if (' ' <= a_ch && a_ch <= '~') {
						return true;
					} else {
						switch (a_ch) {
						case '\t':
						case '\n':
							return true;
						default:
							return false;
						}
					}
				};

				const auto str = static_cast<const char*>(a_ptr);
				constexpr std::size_t max = 1000;
				std::size_t len = 0;
				for (; len < max && str[len] != '\0'; ++len) {
					if (!printable(str[len])) {
						return std::nullopt;
					}
				}

				if (len == 0 || len >= max) {
					return std::nullopt;
				}

				return make_result<String>(std::string_view{ str, len });
			} catch (...) {
				return std::nullopt;
			}
		}

		[[nodiscard]] auto analyze_pointer(
			void* a_ptr,
			std::span<const module_pointer> a_modules) noexcept
			-> analysis_result
		{
			if (auto poly = analyze_polymorphic(a_ptr, a_modules); poly) {
				return *std::move(poly);
			}

			if (auto str = analyze_string(a_ptr); str) {
				return *std::move(str);
			}

			return make_result<Pointer>(a_ptr, a_modules);
		}

		[[nodiscard]] auto analyze_integer(
			std::size_t a_value,
			std::span<const module_pointer> a_modules) noexcept
			-> analysis_result
		{
			try {
				if (a_value != 0) {
					*reinterpret_cast<const volatile std::byte*>(a_value);
					return analyze_pointer(reinterpret_cast<void*>(a_value), a_modules);
				}
			} catch (...) {}

			return make_result<Integer>(a_value);
		}
	}

	std::vector<std::string> analyze_data(
		std::span<const std::size_t> a_data,
		std::span<const module_pointer> a_modules)
	{
		std::vector<std::string> results;
		results.resize(a_data.size());
		std::for_each(
			std::execution::par_unseq,
			a_data.begin(),
			a_data.end(),
			[&](auto& a_val) {
				const auto result = detail::analyze_integer(a_val, a_modules);
				const auto pos = std::addressof(a_val) - a_data.data();
				results[pos] = std::visit(
					[](const auto& a_analysis) { return a_analysis.name(); },
					result);
			});
		return results;
	}
}
