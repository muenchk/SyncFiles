
#include "Functions.h"

#include <iostream>
#include <filesystem>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

TEST_CASE("test Copy", "[copy]")
{
	std::cout << "\nStarting test...\n";
	auto processors = GENERATE(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
	auto deletewithoutmatch = GENERATE(true, false);
	auto overwriteexisting = GENERATE(true, false);
	auto force = GENERATE(true, false);
	auto rm = GENERATE(false, true);
	Functions func;
	func.Copy(L"../../tests", L"../../tests_out", deletewithoutmatch, overwriteexisting, force, false, processors);

	boost::unordered_set<std::wstring> filesinput = Functions::GetFilesRelative(L"../../tests");
	boost::unordered_set<std::wstring> filesoutput = Functions::GetFilesRelative(L"../../tests_out");
	std::filesystem::path inp(L"../../tests");
	std::filesystem::path outp(L"../../tests_out");

	for (auto file : filesinput) {
		REQUIRE(filesoutput.contains(file));
		REQUIRE(std::filesystem::file_size(inp / file) == std::filesystem::file_size(outp / file));
	}

	if (rm)
		std::filesystem::remove_all(L"../../tests_out");
}
