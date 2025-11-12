#pragma once
#include "Types.h"
#include <string>
#include <vector>
#include <thread>
#include <filesystem>
#include <unordered_set>
#include <set>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include <boost/unordered_set.hpp>

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
	boost::unordered_set<std::wstring> IDirSet;
	boost::unordered_set<std::wstring> OFilesSet;
	boost::unordered_set<std::wstring> ODirSet;

	std::deque<std::wstring> filesinput;
	std::deque<std::wstring> dirsinput;
	std::deque<std::wstring> filesoutput;
	std::deque<std::wstring> dirsoutput;

	std::deque<std::wstring> dirstmp;

	std::shared_mutex barrier;

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
	static void GetFiles(std::filesystem::path inputPath, std::deque<std::wstring>& outfiles, std::deque<std::wstring>& outdirs);
	static boost::unordered_set<std::wstring> GetFilesRelative(std::filesystem::path inputPath);
	static void GetFilesRelative(std::filesystem::path inputPath, boost::unordered_set<std::wstring>& files, boost::unordered_set<std::wstring>& dirs);

	void Copy(std::filesystem::path inputPath, std::filesystem::path outputPath, bool deletewithoutmatch, bool overwriteexisting, bool force, bool move, int processors);

	void ReconstitueSymlinks(std::vector<std::filesystem::path> folders);

	void Wait();

	bool IsFinished();

	~Functions();

	std::atomic<size_t> _bytesToCopy = 0;
	std::atomic<size_t> _filesToCopy = 0;
	std::atomic<size_t> _bytesCopied = 0;
	std::atomic<size_t> _filesCopied = 0;

	bool _activeCopy = false;

	ts_deque<std::string> errors;
};

namespace Utility
{

	/// <summary>
	/// Formats microseconds into a proper time string
	/// </summary>
	/// <returns></returns>
	std::string FormatTime(int64_t microseconds);

	/// <summary>
	/// Formats nanoseconds into a proper time string
	/// </summary>
	/// <returns></returns>
	std::string FormatTimeNS(int64_t nanoseconds);
}
