#pragma once

#include "Core.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <iostream>

namespace flaw {
    class ThreadPool {
    public:
      ThreadPool(int32_t threadCount);
      ~ThreadPool();

      void EnqueueTask(std::function<void()> task);
		
    private:
		  static void WorkerThread(ThreadPool* pool);

    private:
      std::mutex _mutex;
      std::condition_variable _conditionVariable;
      std::vector<std::thread> _threads;

      std::queue<std::function<void()>> _tasks;

      bool _stopSignal;
    };
}

