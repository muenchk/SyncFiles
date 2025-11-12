#pragma once

#include <stdlib.h>
#include <string>
#include <span>
#include <memory>
#include <vector>
#include <boost/stacktrace.hpp>
#include <string_view>
#include "PCH.h"

namespace Crash
{
	// from CommonlibSSE : include/RE/RTTI.h
	namespace msvc
	{
		class __declspec(novtable) type_info
		{
		public:
			virtual ~type_info();  // 00

			[[nodiscard]] const char* mangled_name() const noexcept { return _name; }

		private:
			// members
			void* _data;    // 08
			char _name[1];  // 10
		};
		static_assert(sizeof(type_info) == 0x18);
	}

	namespace RTTI
	{
		template <class T>
		class RVA
		{
		public:
			using value_type = T;
			using pointer = value_type*;
			using reference = value_type&;

			constexpr RVA() noexcept = default;

			constexpr RVA(std::uint32_t a_rva) noexcept :
				_rva(a_rva)
			{}

			[[nodiscard]] pointer get() const { return nullptr; }
			[[nodiscard]] std::uint32_t offset() const noexcept { return _rva; }
			[[nodiscard]] reference operator*() const { return *get(); }
			[[nodiscard]] pointer operator->() const { return get(); }
			[[nodiscard]] explicit constexpr operator bool() const noexcept { return is_good(); }

		protected:
			[[nodiscard]] constexpr bool is_good() const noexcept { return _rva != 0; }

			// members
			std::uint32_t _rva{ 0 };  // 00
		};

		using TypeDescriptor = msvc::type_info;

		struct PMD
		{
		public:
			// members
			std::int32_t mDisp;  // 0
			std::int32_t pDisp;  // 4
			std::int32_t vDisp;  // 8
		};

		struct BaseClassDescriptor
		{
		public:
			enum class Attribute : std::uint32_t
			{
				kNone = 0,
				kNotVisible = 1 << 0,
				kAmbiguous = 1 << 1,
				kPrivate = 1 << 2,
				kPrivateOrProtectedBase = 1 << 3,
				kVirtual = 1 << 4,
				kNonPolymorphic = 1 << 5,
				kHasHierarchyDescriptor = 1 << 6
			};

			// members
			RVA<TypeDescriptor> typeDescriptor;                     // 00
			std::uint32_t numContainedBases;                        // 04
			PMD pmd;                                                // 08
			stl::enumeration<Attribute, std::uint32_t> attributes;  // 14
		};

		struct ClassHierarchyDescriptor
		{
		public:
			enum class Attribute
			{
				kNoInheritance = 0,
				kMultipleInheritance = 1 << 0,
				kVirtualInheritance = 1 << 1,
				kAmbiguousInheritance = 1 << 2
			};

			// members
			std::uint32_t signature;                                // 00
			stl::enumeration<Attribute, std::uint32_t> attributes;  // 04
			std::uint32_t numBaseClasses;                           // 08
			RVA<BaseClassDescriptor> baseClassArray;                // 0C
		};

		struct CompleteObjectLocator
		{
		public:
			enum class Signature
			{
				x86 = 0,
				x64 = 1
			};

			// members
			stl::enumeration<Signature, std::uint32_t> signature;  // 00
			std::uint32_t offset;                                  // 04
			std::uint32_t ctorDispOffset;                          // 08
			RVA<TypeDescriptor> typeDescriptor;                    // 0C
			RVA<ClassHierarchyDescriptor> classDescriptor;         // 10
		};
	}

	namespace Modules
	{
		namespace detail
		{
			class Factory;
		}

		class Module
		{
		public:
			virtual ~Module() noexcept = default;

			[[nodiscard]] std::uintptr_t address() const noexcept { return reinterpret_cast<std::uintptr_t>(_image.data()); }

			[[nodiscard]] std::string frame_info(const boost::stacktrace::frame& a_frame) const;

			// Return std::string of assembly for a_ptr
			[[nodiscard]] std::string assembly(const void* a_ptr) const;

			[[nodiscard]] bool in_range(const void* a_ptr) const noexcept
			{
				const auto ptr = reinterpret_cast<const std::byte*>(a_ptr);
				return _image.data() <= ptr && ptr < _image.data() + _image.size();
			}

			[[nodiscard]] bool in_data_range(const void* a_ptr) const noexcept
			{
				const auto ptr = reinterpret_cast<const std::byte*>(a_ptr);
				return _data.data() <= ptr && ptr < _data.data() + _data.size();
			}

			[[nodiscard]] bool in_rdata_range(const void* a_ptr) const noexcept
			{
				const auto ptr = reinterpret_cast<const std::byte*>(a_ptr);
				return _rdata.data() <= ptr && ptr < _rdata.data() + _rdata.size();
			}

			[[nodiscard]] std::string_view name() const { return _name; }
			[[nodiscard]] std::string_view path() const { return _path; }

			[[nodiscard]] const msvc::type_info* type_info() const { return _typeInfo; }

		protected:
			friend class detail::Factory;

			Module(std::string a_name, std::span<const std::byte> a_image);
			Module(std::string a_name, std::span<const std::byte> a_image, std::string a_path);
			[[nodiscard]] virtual std::string get_frame_info(const boost::stacktrace::frame& a_frame) const;

		private:
			std::string _name;
			std::span<const std::byte> _image;
			std::span<const std::byte> _data;
			std::span<const std::byte> _rdata;
			const msvc::type_info* _typeInfo{ nullptr };
			std::string _path;
		};

		[[nodiscard]] auto get_loaded_modules()
			-> std::vector<std::unique_ptr<Module>>;
	}

	using module_pointer = std::unique_ptr<Modules::Module>;
}
