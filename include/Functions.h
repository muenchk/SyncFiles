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
	static boost::unordered_set<std::wstring> GetFilesRelative(std::filesystem::path inputPath);

	void Copy(std::filesystem::path inputPath, std::filesystem::path outputPath, bool deletewithoutmatch, bool overwriteexisting, bool force, bool move, int processors);

	void Wait();

	bool IsFinished();

	~Functions();

	std::atomic<size_t> _bytesToCopy = 0;
	std::atomic<size_t> _filesToCopy = 0;
	std::atomic<size_t> _bytesCopied = 0;
	std::atomic<size_t> _filesCopied = 0;

	ts_deque<std::string> errors;
};

/// <summary>
/// Formats microseconds into a proper time string
/// </summary>
/// <returns></returns>
static std::string FormatTime(int64_t microseconds)
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

/// <summary>
/// Formats nanoseconds into a proper time string
/// </summary>
/// <returns></returns>
static std::string FormatTimeNS(int64_t nanoseconds)
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
