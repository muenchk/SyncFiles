#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>
#include <unordered_set>
#include <semaphore>
#include <atomic>
#include <chrono>
#include <locale>
#include <codecvt>
#include <thread>
#include <cstring>
#include "Types.h"
#include "Functions.h"



std::string GetSize(size_t value)
{
	char buf[20];
	if (value < 1024)  // Bytes
		sprintf(buf, "%7f B ", (double(value)));
	else if (value < 1048576)  // KiloBytes
		sprintf(buf, "%7f KB", (double(value) / 1024));
	else if (value < 1073741824)  // MegaBytes
		sprintf(buf, "%7f MB", (double(value) / 1048576));
	else if (value < 1099511627776)  // GigaBytes
		sprintf(buf, "%7f GB", (double(value) / 1073741824));
	else if (value < 1125899906842624)  // TeraBytes
		sprintf(buf, "%7f TB", (double(value) / 1099511627776));
	else  // petabytes
		sprintf(buf, "%7f PB", (double(value) / 1125899906842624));
	return std::string(buf);
}


auto ConvertToString(std::wstring_view a_in) noexcept
	-> std::optional<std::string>
{
	const wchar_t* input = a_in.data();

	// Count required buffer size (plus one for null-terminator).
	size_t size = (wcslen(input) + 1) * sizeof(wchar_t);
	std::string out(size, '\0');

	size_t convertedSize;
	//wcstombs_s(&convertedSize, out.data(), size, input, size);
	convertedSize = wcstombs(out.data(), input, size);

	if (strcmp(out.c_str(), ""))
		return std::nullopt;

	return out;
}

 auto ConvertToWideString(std::string_view a_in) noexcept
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

std::string ToLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return (unsigned char)std::tolower(c); }  // correct
	);
	return s;
}

std::wstring ToLower(std::wstring s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		[](wchar_t c) { return (wchar_t)std::tolower(c); }  // correct
	);
	return s;
}

void Search(std::filesystem::path path, std::string name)
{
	std::vector<std::wstring> inputs;
	auto inputiter = std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::follow_directory_symlink);
	std::wstring wname = ToLower(ConvertToWideString(name).value());
	long count = 0;
	try
	{
		for (auto const& dir_entry : inputiter) {
			if (dir_entry.is_directory()) {
				if (ToLower(dir_entry.path().filename().wstring()).find(wname) != std::string::npos) {
					//printf("%ws\n", dir_entry.path().wstring().c_str());
					inputs.push_back(dir_entry.path().wstring());
				}
			} else {
				if (ToLower(dir_entry.path().filename().wstring()).find(wname) != std::string::npos) {
					//printf("%ws\n", dir_entry.path().wstring().c_str());
					inputs.push_back(dir_entry.path().wstring());
				}
			}
			count++;
			if (count % 10000 == 0)
				printf("Still Searching...\n");
		}
	}
	catch (std::filesystem::filesystem_error& e) {
		printf("ERROR: %s\n", e.what());
	}
	printf("\n\n\nFOUND\n\n\n");
	for (auto const& entry : inputs)
	{
		printf("%ws\n", entry.c_str());
	}
}

int main(int argc, char** argv)
{
	std::string sInput;
	std::string sOutput;
	std::filesystem::path pathInput;
	std::filesystem::path pathOutput;

	if (argc < 3) {
		printf("Format SyncFiles [OPTIONS] [INPUTPATH] [OUTPUTPATH ~ NAME] expected.\n");
		printf("Options:\n");
		printf("-s\t\tSearches for the filename\n");
		printf("-d\t\tDelete files from Output folder that do not have a match in the input folder\n");
		printf("-o\t\tOverwrite older files in the Outut folder\n");
		printf("-f\t\tOverwrite all files in the Output folder\n");
		printf("-m\t\tDeletes source files, after successful copy operation\n");
		printf("-rm\t\tDeletes files / folders\n");
		printf("--debug\tPrints debug information\n");
		printf("-p<NUM>\tNumber of processors to use\n");
		exit(1);
	}
	bool deletewithoutmatch = false;
	bool overwriteexisting = false;
	bool move = false;
	bool remove = false;
	bool force = false;
	int processors = 1;
	bool debug = false;
	bool search = false;
	bool reconstitutesymlinks = false;
	std::vector<std::string> sinodes;
	for (int i = 1; i < argc; i++) {
		size_t pos = 0;
		std::string option = std::string(argv[i]);
		if (option.find("--debug") != std::string::npos)
			debug = true;
		else if (option.find("-reconstitutesymlinks") != std::string::npos)
			reconstitutesymlinks = true;
		else if (option.find("-rm") != std::string::npos)
			remove = true;
		else if (option.find("-d") != std::string::npos)
			deletewithoutmatch = true;
		else if (option.find("-s") != std::string::npos)
			search = true;
		else if (option.find("-f") != std::string::npos)
			force = true;
		else if (option.find("-o") != std::string::npos)
			overwriteexisting = true;
		else if (option.find("-m") != std::string::npos)
			move = true;
		else if (pos = option.find("-p"); pos != std::string::npos)
			{
				try {
					processors = std::stoi(option.substr(pos+2, option.size() - 2-pos));
				}
			catch (std::exception&) {

			}
		}
		else
		{
			sinodes.push_back(option);
		}
	}
	if (force)
		overwriteexisting = true;
	printf("Search for filename:              %d\n", search);
	printf("Delete files without match:       %d\n", deletewithoutmatch);
	printf("Overwrite existing older file:    %d\n", overwriteexisting);
	printf("Overwrite all files:              %d\n", force);
	printf("Delete source files:              %d\n", move);
	printf("Processors:                       %d\n", processors);
	printf("Delete files:                     %d\n", remove);

	sInput = std::string(argv[argc - 2]);
	pathInput = std::filesystem::path(sInput);

	std::vector<std::filesystem::path> rfiles;
	std::vector<std::filesystem::path> rdirs;
	// parse files / folders
	{
		for (auto& str : sinodes)
		{
			if (std::filesystem::exists(str)) {
				auto entry = std::filesystem::absolute(std::filesystem::path(str));
				if (std::filesystem::is_directory(str)) {
					rdirs.push_back(entry);
					std::cout << "Folder:  " << entry.string() << "\n";
				} else {
					rfiles.push_back(entry);
					std::cout << "File:  " << entry.string() << "\n";
				}
			} else
				std::cout << "Cannot find path \"" + str + "\"\n";
		}
		sinodes.clear();
	}

	if (search) {
		if (!std::filesystem::exists(pathInput)) {
			printf("Input path is empty\n");
			exit(1);
		}
		std::string name = std::string(argv[argc - 1]);
		Search(pathInput, name);
	} else if (remove) {
		std::cout << "Do you really want to delete the files? [Y/N]";
		std::string resp;
		std::cin >> resp;
		if (ToLower(resp) != "y")

		{
			std::cout << "Aborting...\n";
			std::exit(0);
		} else {
			std::cout << "Deleting Files...\n";
		}
		try {
			for (auto pth : rfiles)
				std::filesystem::remove(pth);
			for (auto pth : rdirs)
				std::filesystem::remove_all(pth);
		} catch (std::exception& e) {
			std::cout << e.what() << "\n";
		}
		std::cout << "Deleted all files.\n";
	} else if (reconstitutesymlinks) {
		// we will traverse a target directory looking for symlinks and replace them with files / folders that can be found in one of the other given folders in order of folders given
		Functions func;
		std::thread th1([&func, rdirs]() {
			func.ReconstitueSymlinks(rdirs);
		});

		th1.join();

	} else {
		if (!std::filesystem::exists(pathInput)) {
			printf("Input path is empty\n");
			exit(1);
		}
		sOutput = std::string(argv[argc - 1]);
		pathOutput = std::filesystem::path(sOutput);


		Functions func = Functions();
		std::thread th([&func, &pathInput, &pathOutput, &deletewithoutmatch, &overwriteexisting, &force, &move, &processors]() {
			func.Copy(pathInput, pathOutput, deletewithoutmatch, overwriteexisting, force, move, processors);
		});


		bool finished = false;
		while (!finished) {
			if (func.IsFinished()) {
				finished = true;
			}
			if (func._activeCopy)
				printf("Written Files:\t%5llu / %5llu\t\tSizeWritten:\t%llu / %llu\t %d%%\n", func._filesCopied.load(), func._filesToCopy.load(), func._bytesCopied.load(), func._bytesToCopy.load(), (int)((double)func._bytesCopied.load() / (double)func._bytesToCopy.load() * 100));

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
		th.join();
		printf("Written Files:\t%5llu / %5llu\t\tSizeWritten:\t%llu / %llu\t %d%%\n", func._filesCopied.load(), func._filesToCopy.load(), func._bytesCopied.load(), func._bytesToCopy.load(), (int)((double)func._bytesCopied.load() / (double)func._bytesToCopy.load() * 100));

		printf("Errors: %zd\n", func.errors.size());
		for (size_t i = 0; i < func.errors.size(); i++) {
			printf("%s\n", func.errors[i].c_str());
		}
	}

	return 0;
}
