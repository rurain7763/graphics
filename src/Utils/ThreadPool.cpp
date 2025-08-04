#include "pch.h"
#include "ThreadPool.h"

namespace flaw {
	ThreadPool::ThreadPool(int32_t threadCount) 
		: _stopSignal(false)
	{
		for (int32_t i = 0; i < threadCount; ++i) {
			_threads.emplace_back(&ThreadPool::WorkerThread, this);
		}
	}

	ThreadPool::~ThreadPool() {
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_stopSignal = true;
		}

		_conditionVariable.notify_all();

		for (auto& thread : _threads) {
			thread.join();
		}
	}

	void ThreadPool::EnqueueTask(std::function<void()> task) {
		{
			std::lock_guard<std::mutex> lock(_mutex);
			_tasks.push(std::move(task));
		}

		_conditionVariable.notify_one();
	}

	void ThreadPool::WorkerThread(ThreadPool* pool) {
		while (true) {
			std::function<void()> task;

			{
				std::unique_lock<std::mutex> lock(pool->_mutex);
				pool->_conditionVariable.wait(lock, [pool] { return !pool->_tasks.empty() || pool->_stopSignal; });
				if (pool->_stopSignal && pool->_tasks.empty()) {
					return; // Exit the thread
				}
				task = std::move(pool->_tasks.front());
				pool->_tasks.pop();
			}

			try {
				task();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in thread: " << e.what() << std::endl;
			}
		}
	}
}
