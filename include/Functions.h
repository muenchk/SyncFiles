#include "ThreadSafe.h"
#include <memory>
#include <mutex>
#include <chrono>

void TraverseINodes(bool* working, ts_deque<std::filesystem::path>* queue, ts_deque<std::filesystem::path>* dirs, ts_deque<std::filesystem::path>* files, bool* stop_traverse, std::condition_variable* sync)
{
	std::filesystem::path pth;
	while (*stop_traverse == false)
	{
		try {
			pth = queue->get_pop_front();
			*working = true;
		}
		catch (std::exception& e) {
			*working = false;
		}
		if (*working)
		{
			std::filesystem::directory_iterator(pth);
			for (const auto& entry : std::filesystem::directory_iterator(pth))
			{
				if (entry.exists() && !entry.path().empty()) {
					if (std::filesystem::is_directory(entry.path()))
						queue->push_back(entry.path());
				}
			}
		}
	}
}

void DeleteINodes(ts_deque<std::filesystem::path>* dirs, ts_deque<std::filesystem::path>* files, bool* stop_delete, std::condition_variable* sync)
{
	std::mutex mut;
	std::unique_lock<std::mutex> guard(mut);
	while (*stop_delete == false)
	{
		sync->wait_for(guard, std::chrono::milliseconds(10), [stop_delete, dirs, files] { *stop_delete == true || dirs->empty() == false || files->empty() == false; });
		bool found = true;
		std::filesystem::path pth;
		while (found)
		{
			try {
				pth = files->get_pop_front();
				found = true;
			}
			catch (std::exception& e) {
				found = false;
			}
			if (found)
				std::filesystem::remove_all(pth);
		}
		found = true;
		while (found) {
			try {
				pth = dirs->get_pop_front();
				found = true;
			} catch (std::exception& e) {
				found = false;
			}
			if (found)
				std::filesystem::remove_all(pth);
		}
	}
}