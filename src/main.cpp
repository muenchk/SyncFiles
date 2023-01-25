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

template <typename T >
	class ThreadSafeVar
{
	T value;
	std::atomic_flag locked = ATOMIC_FLAG_INIT;

	public:

	T Get()
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		T val = value;
		locked.clear(std::memory_order_release);
		return val;
	}

	void Set(T val)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value = val;
		locked.clear(std::memory_order_release);
	}

	void Increment(T val)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value += val;
		locked.clear(std::memory_order_release);
	}

	void Decrement(T val)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value -= val;
		locked.clear(std::memory_order_release);
	}
};

template <typename T>
class ThreadSafeVarVector
{
	std::vector<T> value;
	std::atomic_flag locked = ATOMIC_FLAG_INIT;

	public:

	T Get(size_t index)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		T val;
		if (value.size() > index)
			val = value[index];
		locked.clear(std::memory_order_release);
		return val;
	}

	void Set(T val, size_t index)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		if (value.size() > index)
			value[index] = val;
		locked.clear(std::memory_order_release);
	}

	void Push(T val)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value.push_back(val);
		locked.clear(std::memory_order_release);
	}

	void Pop()
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value.pop_back();
		locked.clear(std::memory_order_release);
	}

	size_t Size()
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		size_t size = value.size();
		locked.clear(std::memory_order_release);
		return size;
	}
};

std::vector<std::wstring>* tocopy = nullptr;
std::wstring inputprefix;
std::wstring outputprefix;

ThreadSafeVar<int> copied;
ThreadSafeVar<uintmax_t> bytescopied;
ThreadSafeVarVector<std::string> errors;
ThreadSafeVar<int> finishedthreads;

void CopyFile(std::string source, std::string dest, char* buffer, size_t bufsize)
{
	std::ifstream in(source, std::ios_base::binary);
	std::ofstream out(dest, std::ios_base::binary | std::ios_base::trunc);
	size_t ss = std::filesystem::file_size(source);
	size_t read = 0;
	if (in.is_open() && out.is_open()) {
		try {
			while (ss - read > bufsize) {
				in.read(buffer, bufsize);
				out.write((char*)buffer, bufsize);
				read += bufsize;
			}
			if (ss - read > 0) {
				in.read(buffer, ss - read);
				out.write(buffer, ss - read);
			}
		} catch (std::ios_base::failure& e) {
			errors.Push("[ERROR] [Copy File] " + std::string(e.what()));
		}
	} else {
		errors.Push("[ERROR] [Copy File] cannot open files");
	}
	copied.Increment(1);
	bytescopied.Increment(ss);
}

void Copy(size_t begin, size_t end)
{
	std::error_code err;
	//size_t bufsize = 10485760;
	//char* buffer = new char[10485760];
	for (size_t i = begin; i < end && i < tocopy->size(); i++) {
		// copy file
		try {
			//CopyFile(inputprefix + tocopy->at(i), outputprefix + tocopy->at(i), buffer, bufsize);
			std::filesystem::copy_file(inputprefix + tocopy->at(i), outputprefix + tocopy->at(i), std::filesystem::copy_options::overwrite_existing);
			copied.Increment(1);
			bytescopied.Increment(std::filesystem::file_size(inputprefix + tocopy->at(i), err));
		}
		catch (std::filesystem::filesystem_error& e) {
			errors.Push("[ERROR] [Copy File] " + std::string(e.what()));
		}
	}
	//delete[] buffer;
	finishedthreads.Increment(1);
}

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

int main(int argc, char** argv)
{
	std::string sInput;
	std::string sOutput;
	std::filesystem::path pathInput;
	std::filesystem::path pathOutput;

	if (argc < 3) {
		printf("Format SyncFiles [OPTIONS] [INPUTPATH] [OUTPUTPATH] expected.\n");
		printf("Options:\n");
		printf("-d\t\tDelete files from Output folder that do not have a match in the input folder\n");
		printf("-o\t\tOverwrite older files in the Outut folder\n");
		printf("-f\t\tOverwrite all files in the Output folder\n");
		printf("-p<NUM>\tNumber of processors to use\n");
		exit(1);
	}
	bool deletewithoutmatch = false;
	bool overwriteexisting = false;
	bool force = false;
	int processors = 1;
	for (int i = 1; i < argc - 2; i++) {
		size_t pos = 0;
		std::string option = std::string(argv[i]);
		if (option.find("-d") != std::string::npos)
			deletewithoutmatch = true;
		else if (option.find("-f") != std::string::npos)
			force = true;
		else if (option.find("-o") != std::string::npos)
			overwriteexisting = true;
		else if (pos = option.find("-p"); pos != std::string::npos)
			{
				try {
					processors = std::stoi(option.substr(pos+2, option.size() - 2-pos));
				}
			catch (std::exception&) {

			}
		}
	}
	if (force)
		overwriteexisting = true;
	printf("Delete files without match:       %d\n", deletewithoutmatch);
	printf("Overwrite existing older file:    %d\n", overwriteexisting);
	printf("Overwrite all files:              %d\n", force);
	printf("Processors:                       %d\n", processors);

	sInput = std::string(argv[argc-2]);
	sOutput = std::string(argv[argc-1]);
	pathInput = std::filesystem::path(sInput);
	pathOutput = std::filesystem::path(sOutput);

	if (!std::filesystem::exists(pathInput)) {
		printf("Input path is empty\n");
		exit(1);
	}

	std::vector<std::wstring> filesinput;
	std::vector<std::wstring> dirsinput;
	std::vector<std::wstring> filesoutput;
	std::vector<std::wstring> dirsoutput;

	std::vector<std::wstring> dirstmp;

	if (std::filesystem::exists(pathOutput) == false) {
		std::filesystem::create_directories(pathOutput);
		// crash if we fail
	}

	printf("Find all files...\n");

	//size_t count = 0;

	auto inputiter = std::filesystem::recursive_directory_iterator(pathInput, std::filesystem::directory_options::follow_directory_symlink);
	auto outputiter = std::filesystem::recursive_directory_iterator(pathOutput, std::filesystem::directory_options::follow_directory_symlink);
	// iterate over all entries
	try {
		for (auto const& dir_entry : inputiter) {
			//printf("%zd %ws\n", count, dir_entry.path().wstring().c_str());
			//count++;
			if (dir_entry.is_directory())
				dirsinput.push_back(dir_entry.path().wstring());
			else
				filesinput.push_back(dir_entry.path().wstring());
		}
		for (auto const& dir_entry : outputiter) {
			if (dir_entry.is_directory())
				dirsoutput.push_back(dir_entry.path().wstring());
			else
				filesoutput.push_back(dir_entry.path().wstring());
		}
	}
	catch (std::filesystem::filesystem_error& e) {
		printf("ERROR: %s\n", e.what());
	}

	inputprefix = pathInput.wstring() + std::filesystem::path::preferred_separator;
	outputprefix = pathOutput.wstring() + std::filesystem::path::preferred_separator;
	int inputprefixlength = (int)inputprefix.length();
	int outputprefixlength = (int)outputprefix.length();
	
	printf("Calculate files...\n");

	std::unordered_set<std::wstring> IFilesSet;
	std::unordered_set<std::wstring> IDirSet;
	std::unordered_set<std::wstring> OFilesSet;
	std::unordered_set<std::wstring> ODirSet;

	std::vector<std::wstring> overwrite;
	std::vector<std::wstring> newer;
	std::vector<std::wstring> identical;

	printf("check1\n");
	// remove prefixes from all pathes
	for (int i = 0; i < filesinput.size(); i++) {
		IFilesSet.insert(filesinput[i].substr(inputprefixlength, filesinput[i].size() - inputprefixlength));
	}
	printf("check2\n");
	for (int i = 0; i < dirsinput.size(); i++) {
		IDirSet.insert(dirsinput[i].substr(inputprefixlength, dirsinput[i].size() - inputprefixlength));
	}
	printf("check3\n");
	for (int i = 0; i < filesoutput.size(); i++) {
		OFilesSet.insert(filesoutput[i].substr(outputprefixlength, filesoutput[i].size() - outputprefixlength));
	}
	printf("check4\n");
	for (int i = 0; i < dirsoutput.size(); i++) {
		ODirSet.insert(dirsoutput[i].substr(outputprefixlength, dirsoutput[i].size() - outputprefixlength));
	}

	std::filesystem::file_time_type ITime;
	std::filesystem::file_time_type OTime;
	printf("check5\n");
	std::error_code err;

	// check for existing files
	// remaining in IFiles are new files
	// remaining in OFiles are those to delete
	for (auto const& file : IFilesSet) {
		//printf("%ws\n", file.c_str());
		if (OFilesSet.contains(file)) {
			ITime = std::filesystem::last_write_time(inputprefix + file, err);
			//printf("%s\n", err.message().c_str());
			OTime = std::filesystem::last_write_time(outputprefix + file, err);
			//printf("%s\n", err.message().c_str());
			if (ITime > OTime) {
				overwrite.push_back(file);
			} else if (ITime < OTime) {
				newer.push_back(file);
			} else {
				identical.push_back(file);
			}
			IFilesSet.erase(file);
			OFilesSet.erase(file);
		}
	}
	printf("check6\n");
	// check for exsisting dirs
	// remaining in IDir are new dirs
	// remaining in ODir are those to delete
	for (auto const& dir : IDirSet) {
		if (ODirSet.contains(dir)) {
			IDirSet.erase(inputprefix + dir);
			ODirSet.erase(outputprefix + dir);
		}
	}
	printf("check7\n");
	
	int cdeleted = 0;

	if (deletewithoutmatch) {
		printf("Deleting files without match...\n");
		for (auto const& file : OFilesSet) {
			try {
				std::filesystem::remove(outputprefix + file);
				cdeleted++;
			}
			catch (std::filesystem::filesystem_error& e) {
				errors.Push("[ERROR] [Delete File] " + std::string(e.what()));
			}
		}
		printf("Deleting folder without math....\n");
		for (auto const& dir : ODirSet) {
			try {
				std::filesystem::remove_all(outputprefix + dir);
			}
			catch (std::filesystem::filesystem_error&) {
				// don't output, since we could try to delete folders we already deleted, since we are in alphabetical order
			}
		}
	}

	tocopy = new std::vector<std::wstring>;
	int nocopy = 0;
	size_t bytestopcopy = 0;
	printf("Calculating files to copy...\n");
	// new files
	printf("New Files:         %5zd\n", IFilesSet.size());
	for (auto const& file : IFilesSet) {
		tocopy->push_back(file);
		bytestopcopy += std::filesystem::file_size(std::filesystem::path(inputprefix + file), err);
	}
	// overwrite
	printf("Overwrite Files:   %5zd\n", overwrite.size());
	if (overwriteexisting) {
		for (auto const& file : overwrite) {
			tocopy->push_back(file);
			bytestopcopy += std::filesystem::file_size(std::filesystem::path(inputprefix + file), err);
		}
	} else
		nocopy += (int)overwrite.size();
	// newer
	printf("Newer Files:       %5zd\n", newer.size());
	if (force) {
		for (auto const& file : newer) {
			tocopy->push_back(file);
			bytestopcopy += std::filesystem::file_size(std::filesystem::path(inputprefix + file), err);
		}
	} else {
		nocopy += (int)newer.size();
	}
	// indetical files are not copied
	printf("Identical Files:   %5zd\n", identical.size());
	printf("To Copy:\t%5zd / %5zd\n", tocopy->size(), tocopy->size() + nocopy);


	// create directories
	for (auto const& dir : IDirSet) {
		try {
			std::filesystem::create_directories(std::filesystem::path(outputprefix + dir), err);
		} catch (std::filesystem::filesystem_error& e) {
			errors.Push("[ERROR] [Create Directory] " + std::string(e.what()));
		}
	}

	// start copy threads
	size_t begin = 0;
	size_t end = tocopy->size();
	std::vector<std::thread*> threads;
	size_t slice = end / processors;
	slice += 1;
	for (int i = 1; begin < end && i <= processors; i++) {
		if (i == processors) {
			threads.push_back(new std::thread(Copy, begin, end));
			threads[threads.size() - 1]->detach();
		}
		else {
			threads.push_back(new std::thread(Copy, begin, begin + slice));
			threads[threads.size() - 1]->detach();
			begin += slice;
		}
	}

	bool finished = false;
	while (!finished) {
		if (finishedthreads.Get() == threads.size()) {
			finished = true;
		}

		printf("Written Files:\t%5d / %5zd\t\tSizeWritten:\t%s / %s\n", copied.Get(), tocopy->size(), GetSize(bytescopied.Get()).c_str(), GetSize(bytestopcopy).c_str());

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	
	printf("Errors: %zd\n", errors.Size());
	for (size_t i = 0; i < errors.Size(); i++) {
		printf("%s\n", errors.Get(i).c_str());
	}

	return 0;
}