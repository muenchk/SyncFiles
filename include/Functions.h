#pragma once
#include "Types.h"
#include <string>
#include <vector>
#include <thread>
#include <filesystem>
#include <unordered_set>
#include <memory>
#include <mutex>

class Functions
{
private:
	ts_deque<std::wstring> _copyQueue;
	ts_deque<std::wstring> _deleteQueue;
	ts_deque<std::wstring> _createDirsQueue;
	bool _doneCreatingDirs = false;
	bool _doneStuff = false;
	std::condition_variable _waiterCond;
	std::mutex _waiter;
	std::wstring _inputPrefix;
	std::wstring _outputPrefix;

	std::vector<std::thread> _threads;

	ts_deque<std::wstring> IFilesSet;
	std::unordered_set<std::wstring> IDirSet;
	std::unordered_set<std::wstring> OFilesSet;
	std::unordered_set<std::wstring> ODirSet;

	std::deque<std::wstring> filesinput;
	std::deque<std::wstring> dirsinput;
	std::deque<std::wstring> filesoutput;
	std::deque<std::wstring> dirsoutput;

	std::deque<std::wstring> dirstmp;

	int outputprefixlength = 0;
	int inputprefixlength = 0;

	int cdeleted = 0;
	
	bool _move = false;
	bool _force = false;
	bool _overwriteexisting = false;

	void Helper_IFiles();
	void Helper_OFiles();
	void Helper_IDirSet();
	void Helper_ODirSet();

	void Helper_SortFiles();

	void Helper_CreateDirs();

	void DoStuff();

	bool _finished = false;

public:
	void Copy(std::filesystem::path inputPath, std::filesystem::path outputPath, bool deletewithoutmatch, bool overwriteexisting, bool force, bool move, int processors);

	void Wait();

	bool IsFinished();

	std::atomic<size_t> _bytesToCopy = 0;
	std::atomic<size_t> _filesToCopy = 0;
	std::atomic<size_t> _bytesCopied = 0;
	std::atomic<size_t> _filesCopied = 0;

	ts_deque<std::string> errors;
};