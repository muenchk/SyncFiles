#include "Functions.h"
#include <functional>
#include <chrono>
#include <iostream>

Functions::~Functions()
{
	if (_threads.size() > 0)
		for (std::thread& thread: _threads)
		{
			if (thread.joinable())
				thread.~thread();
		}
	_copyQueue.clear();
	_deleteQueue.clear();
	_createDirsQueue.clear();

	IFilesSet.clear();

	filesinput.clear();
	dirsinput.clear();
	filesoutput.clear();
	dirsoutput.clear();

	dirstmp.clear();

	
	_copyQueue.~ts_deque();
	_deleteQueue.~ts_deque();
	_createDirsQueue.~ts_deque();

	IFilesSet.~ts_deque();
	IDirSet.clear();
	OFilesSet.clear();
	ODirSet.clear();

	filesinput.~deque();
	dirsinput.~deque();
	filesoutput.~deque();
	dirsoutput.~deque();

	dirstmp.~deque();
}

void Functions::Helper_IFiles()
{
	std::shared_lock<std::shared_mutex> guard(barrier);
	for (int i = 0; i < filesinput.size(); i++) {
		IFilesSet.push_back(filesinput[i].substr(inputprefixlength, filesinput[i].size() - inputprefixlength));
	}
}

void Functions::Helper_OFiles() 
{
	std::shared_lock<std::shared_mutex> guard(barrier);
	for (int i = 0; i < filesoutput.size(); i++) {
		OFilesSet.insert(filesoutput[i].substr(outputprefixlength, filesoutput[i].size() - outputprefixlength));
	}
}
void Functions::Helper_IDirSet() 
{
	std::shared_lock<std::shared_mutex> guard(barrier);
	for (int i = 0; i < dirsinput.size(); i++) {
		IDirSet.insert(dirsinput[i].substr(inputprefixlength, dirsinput[i].size() - inputprefixlength));
	}
}
void Functions::Helper_ODirSet() 
{
	std::shared_lock<std::shared_mutex> guard(barrier);
	for (int i = 0; i < dirsoutput.size(); i++) {
		ODirSet.insert(dirsoutput[i].substr(outputprefixlength, dirsoutput[i].size() - outputprefixlength));
	}
}
void Functions::Helper_CreateDirs()
{
	std::error_code err;
	std::unique_lock<std::mutex> guard(_waiter);
	while (!_doneCreatingDirs || _createDirsQueue.size() > 0)
	{
		_waiterCond.wait_for(guard, std::chrono::milliseconds(10), [this]() { return _createDirsQueue.empty() == false || _doneCreatingDirs; });
		try
		{
			std::wstring dir = _createDirsQueue.get_pop_front();
			try {
				std::filesystem::create_directories(std::filesystem::path(_outputPrefix + dir), err);
			} catch (std::filesystem::filesystem_error& e) {
				errors.push_back("[ERROR] [Create Directory] " + std::string(e.what()));
			}
		}
		catch (std::exception&) {

		}
	}
}

void Functions::DoStuff()
{
	std::error_code err;
	std::mutex wait;
	std::unique_lock<std::mutex> guard(wait);
	while (!_doneStuff || !_copyQueue.empty() || !_deleteQueue.empty()) {
		_waiterCond.wait_for(guard, std::chrono::milliseconds(10), [this]() { return _doneStuff || !_copyQueue.empty() || !_deleteQueue.empty(); });
		try {
			std::wstring copy = _copyQueue.get_pop_front();
			try {
				if (_move) {
					std::filesystem::rename(_inputPrefix + copy, _outputPrefix + copy);
				} else {
					std::filesystem::copy_file(_inputPrefix + copy, _outputPrefix + copy, std::filesystem::copy_options::overwrite_existing);
				}
				_filesCopied++;
				_bytesCopied += (std::filesystem::file_size(_outputPrefix + copy, err));
			} catch (std::filesystem::filesystem_error& e) {
				errors.push_back("[ERROR] [Copy File] " + std::string(e.what()));
			}
		} catch (std::exception&) {
		}
		try {
			std::wstring del = _deleteQueue.get_pop_front();
			try {
				std::filesystem::remove(_outputPrefix + del);
				cdeleted++;
			} catch (std::filesystem::filesystem_error& e) {
				errors.push_back("[ERROR] [Delete File] " + std::string(e.what()));
			}
		} catch (std::exception&) {
		}
	}
}

void Functions::Helper_SortFiles()
{
	std::filesystem::file_time_type ITime;
	std::filesystem::file_time_type OTime;
	std::error_code err;
	while (IFilesSet.size() > 0) {
		try {
			std::wstring file = IFilesSet.get_pop_front();
			if (file.empty() == false && OFilesSet.contains(file)) {
				ITime = std::filesystem::last_write_time(_inputPrefix + file, err);
				OTime = std::filesystem::last_write_time(_outputPrefix + file, err);
				// if input file time is newer than output file time
				if (ITime > OTime) {
					_copyQueue.push_back(file);
					_bytesToCopy += std::filesystem::file_size(std::filesystem::path(_inputPrefix + file), err);
					_filesToCopy++;
				// if output file time is newer only overwrite if force is enabled
				} else if (ITime < OTime) {
					if (_force) {
						_copyQueue.push_back(file);
						_bytesToCopy += std::filesystem::file_size(std::filesystem::path(_inputPrefix + file), err);
						_filesToCopy++;
					}
				// if times are identical overwrite is overwriteexisting is enabled, or file sizes are different
				} else {
					if (_overwriteexisting || std::filesystem::file_size(_inputPrefix + file) != std::filesystem::file_size(_outputPrefix + file)) {
						_copyQueue.push_back(file);
						_bytesToCopy += std::filesystem::file_size(std::filesystem::path(_inputPrefix + file), err);
						_filesToCopy++;
					}
				}
				OFilesSet.erase(file);
			} else {
				_copyQueue.push_back(file);
				_bytesToCopy += std::filesystem::file_size(std::filesystem::path(_inputPrefix + file), err);
				_filesToCopy++;
			}
		} catch (std::exception&) {}
	}
}

boost::unordered_set<std::wstring> Functions::GetFilesRelative(std::filesystem::path inputPath)
{
	boost::unordered_set<std::wstring> files;

	if (std::filesystem::exists(inputPath) == false) {
		return files;
	}
	std::wstring _inputPrefix = inputPath.wstring().append(1, std::filesystem::path::preferred_separator);
	int inputprefixlength = (int)_inputPrefix.length();

	std::unordered_set<std::wstring> filesinput;
	std::unordered_set<std::wstring> dirsinput;
	auto inputiter = std::filesystem::recursive_directory_iterator(inputPath, std::filesystem::directory_options::follow_directory_symlink);
	//iterate over all entries
	try {
		for (auto const& dir_entry : inputiter) {
			//printf("%zd %ws\n", count, dir_entry.path().wstring().c_str());
			//count++;
			if (dir_entry.is_directory())
				dirsinput.insert(dir_entry.path().wstring());
			else
				filesinput.insert(dir_entry.path().wstring());
		}
	} catch (std::filesystem::filesystem_error& e) {
		printf("ERROR: %s\n", e.what());
	}
	for (auto file : filesinput) {
		files.insert(file.substr(inputprefixlength, file.size() - inputprefixlength));
	}
	return files;
}
void Functions::GetFiles(std::filesystem::path inputPath, std::deque<std::wstring>& outfiles, std::deque<std::wstring>& outdirs)
{
	if (std::filesystem::exists(inputPath) == false) {
		return;
	}

	auto inputiter = std::filesystem::recursive_directory_iterator(inputPath, std::filesystem::directory_options::follow_directory_symlink);
	//iterate over all entries
	try {
		for (auto const& dir_entry : inputiter) {
			//printf("%zd %ws\n", count, dir_entry.path().wstring().c_str());
			//count++;
			if (dir_entry.is_directory())
				outdirs.push_back(dir_entry.path().wstring());
			else
				outfiles.push_back(dir_entry.path().wstring());
		}
	} catch (std::filesystem::filesystem_error& e) {
		printf("ERROR: %s\n", e.what());
	}
}

void Functions::Copy(std::filesystem::path inputPath, std::filesystem::path outputPath, bool deletewithoutmatch, bool overwriteexisting, bool force, bool move, int processors)
{
	_finished = false;
	_move = move;
	_overwriteexisting = overwriteexisting;
	_force = force;

	if (std::filesystem::exists(outputPath) == false) {
		std::filesystem::create_directories(outputPath);
		// crash if we fail
	}

	printf("Find all files...");
	auto begin = std::chrono::steady_clock::now();

	std::thread thr1([inputPath, outfiles = &filesinput, outdirs = &dirsinput]() {
		GetFiles(inputPath, *outfiles, *outdirs);
	});
	std::thread thr2([outputPath, outfiles = &filesoutput, outdirs = &dirsoutput]() {
		GetFiles(outputPath, *outfiles, *outdirs);
	});
	thr1.join();
	thr2.join();
	std::cout << Utility::FormatTimeNS(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count()).c_str() << "\n";
	// we have found all files, generate prefixes
	printf("Generate prefixes...");
	begin = std::chrono::steady_clock::now();

	_inputPrefix = inputPath.wstring().append(1, std::filesystem::path::preferred_separator);
	_outputPrefix = outputPath.wstring().append(1, std::filesystem::path::preferred_separator);
	inputprefixlength = (int)_inputPrefix.length();
	outputprefixlength = (int)_outputPrefix.length();

	std::cout << Utility::FormatTimeNS(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count()).c_str() << "\n";
	// calculate files we need
	printf("Calculate files...");
	begin = std::chrono::steady_clock::now();

	// remove prefixes from all paths
	{
		std::thread th1 = std::thread(&Functions::Helper_IFiles, this);
		std::thread th2 = std::thread(&Functions::Helper_OFiles, this);
		std::thread th3 = std::thread(&Functions::Helper_IDirSet, this);
		std::thread th4 = std::thread(&Functions::Helper_ODirSet, this);
		th1.join();
		th2.join();
		th3.join();
		th4.join();
	}

	{
		std::unique_lock<std::shared_mutex> guard(barrier);
	}

	std::cout << Utility::FormatTimeNS(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count()).c_str() << "\n";
	printf("Handle directories...");
	begin = std::chrono::steady_clock::now();

	std::error_code err;

	//std::thread thcr = std::thread(&Functions::Helper_CreateDirs, this);
	// 
	for (auto dir : IDirSet) {
		if (ODirSet.contains(dir)) {
			ODirSet.erase(dir);
			//IDirSet.erase(dir);
			continue;
		}
		else
		{
			//IDirSet.erase(dir);
			//_createDirsQueue.push_back(dir);
			try {
				std::filesystem::create_directories(std::filesystem::path(_outputPrefix + dir), err);
			} catch (std::filesystem::filesystem_error& e) {
				errors.push_back("[ERROR] [Create Directory] " + std::string(e.what()));
			}
		}
	}
	IDirSet.clear();
	//_doneCreatingDirs = true;
	//thcr.join();

	std::cout << Utility::FormatTimeNS(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count()).c_str() << "\n";
	printf("Actually Copy...\n");

	for (int i = 0; i < processors; i++)
	{
		_threads.emplace_back(std::thread(&Functions::DoStuff, this));
	}

	// check for existing files
	// remaining in IFiles are new files
	// remaining in OFiles are those to delete
	
	{
		std::thread th1 = std::thread(&Functions::Helper_SortFiles, this);
		std::thread th2 = std::thread(&Functions::Helper_SortFiles, this);
		std::thread th3 = std::thread(&Functions::Helper_SortFiles, this);
		std::thread th4 = std::thread(&Functions::Helper_SortFiles, this);
		th1.join();
		th2.join();
		th3.join();
		th4.join();
	}

	if (deletewithoutmatch) {
		printf("Deleting files without match...");
		begin = std::chrono::steady_clock::now();
		for (auto const& file : OFilesSet) {
			_deleteQueue.push_back(_outputPrefix + file);
			cdeleted++;
		}
		std::cout << Utility::FormatTimeNS(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count()).c_str() << "\n";
	}

	_doneStuff = true;
	
	for (int i = 0; i < processors; i++) {
		_threads[i].join();
	}
	_threads.clear();

	// clean up

	if (deletewithoutmatch) {
		printf("Deleting folder without math...");
		begin = std::chrono::steady_clock::now();
		for (auto const dir : ODirSet) {
			try {
				std::filesystem::remove_all(_outputPrefix + dir);
			} catch (std::filesystem::filesystem_error&) {
				// don't output, since we could try to delete folders we already deleted, since we are in alphabetical order
			}
		}
		ODirSet.clear();
		std::cout << Utility::FormatTimeNS(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count()).c_str() << "\n";
	}

	_finished = true;
}

bool Functions::IsFinished()
{
	return _finished;
}

void Functions::Wait()
{
	for (auto& thr : _threads)
	{
		thr.join();
	}
}

std::string Utility::FormatTime(int64_t microseconds)
{
	std::stringstream ss;
	int64_t tmp = 0;
	if (microseconds > 60000000) {
		tmp = (int64_t)trunc((long double)microseconds / 60000000);
		ss << std::setw(6) << tmp << "m ";
		microseconds -= tmp * 60000000;
	} else
		ss << "        ";
	if (microseconds > 1000000) {
		tmp = (int64_t)trunc((long double)microseconds / 1000000);
		ss << std::setw(2) << tmp << "s ";
		microseconds -= tmp * 1000000;
	} else
		ss << "    ";
	if (microseconds > 1000) {
		tmp = (int64_t)trunc((long double)microseconds / 1000);
		ss << std::setw(3) << tmp << "ms ";
		microseconds -= tmp * 1000;
	} else
		ss << "      ";
	ss << std::setw(3) << microseconds << "μs";
	return ss.str();
}

std::string Utility::FormatTimeNS(int64_t nanoseconds)
{
	std::stringstream ss;
	int64_t tmp = 0;
	if (nanoseconds > 60000000000) {
		tmp = (int64_t)trunc((long double)nanoseconds / 60000000000);
		ss << std::setw(6) << tmp << "m ";
		nanoseconds -= tmp * 60000000000;
	} else
		ss << "        ";
	if (nanoseconds > 1000000000) {
		tmp = (int64_t)trunc((long double)nanoseconds / 1000000000);
		ss << std::setw(2) << tmp << "s ";
		nanoseconds -= tmp * 1000000000;
	} else
		ss << "    ";
	if (nanoseconds > 1000000) {
		tmp = (int64_t)trunc((long double)nanoseconds / 1000000);
		ss << std::setw(3) << tmp << "ms ";
		nanoseconds -= tmp * 1000000;
	} else
		ss << "      ";
	if (nanoseconds > 1000) {
		tmp = (int64_t)trunc((long double)nanoseconds / 1000);
		ss << std::setw(3) << tmp << "μs ";
		nanoseconds -= tmp * 1000;
	} else
		ss << "      ";
	ss << std::setw(3) << nanoseconds << "ns";
	return ss.str();
}
