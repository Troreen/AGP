#pragma once

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include "ThreadSafeQueue.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
//#include <functional>
#include <future>
#include <memory>
#include <thread>
//#include <type_traits>
//#include <utility>
#include <vector>


namespace CUThreadPool
{
	class ThreadPool
	{
	private:
		class IThreadTask
		{
		public:
			IThreadTask(void) = default;
			virtual ~IThreadTask(void) = default;
			IThreadTask(const IThreadTask& rhs) = delete;
			IThreadTask& operator=(const IThreadTask& rhs) = delete;
			IThreadTask(IThreadTask&& other) = default;
			IThreadTask& operator=(IThreadTask&& other) = default;

			virtual void Execute() = 0;
		};

		template <typename Func>
		class ThreadTask : public IThreadTask
		{
		public:
			ThreadTask(Func&& func)
				: myFunc{ std::move(func) }
			{
			}

			~ThreadTask(void) override = default;
			ThreadTask(const ThreadTask& rhs) = delete;
			ThreadTask& operator=(const ThreadTask& rhs) = delete;
			ThreadTask(ThreadTask&& other) = default;
			ThreadTask& operator=(ThreadTask&& other) = default;

			void Execute() override
			{
				myFunc();
			}

		private:
			Func myFunc;
		};

	public:
		template <typename T>
		class TaskFuture
		{
		public:
			TaskFuture(std::future<T>&& future)
				: myFuture{ std::move(future) }
			{
			}

			TaskFuture(const TaskFuture& rhs) = delete;
			TaskFuture& operator=(const TaskFuture& rhs) = delete;
			TaskFuture(TaskFuture&& other) = default;
			TaskFuture& operator=(TaskFuture&& other) = default;
			~TaskFuture(void)
			{
				if (myFuture.valid())
				{
					myFuture.get();
				}
			}

			auto Get(void)
			{
				return myFuture.get();
			}

		private:
			std::future<T> myFuture;
		};

	public:
		ThreadPool(void)
			:ThreadPool{ std::max(std::thread::hardware_concurrency(), 2u) - 1u }
		{
		}
		explicit ThreadPool(const std::uint32_t numThreads)
			: myDone{ false },
			myWorkQueue{},
			myThreads{}
		{
			try
			{
				for (std::uint32_t i = 0u; i < numThreads; ++i)
				{
					myThreads.emplace_back(&ThreadPool::Worker, this);
				}
			}
			catch (...)
			{
				Destroy();
				throw;
			}
		}

		ThreadPool(const ThreadPool& rhs) = delete;
		ThreadPool& operator=(const ThreadPool& rhs) = delete;
		~ThreadPool(void)
		{
			Destroy();
		}

		template <typename Func, typename... Args>
		auto Submit(Func&& func, Args&&... args)
		{
			auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
			using ResultType = std::result_of_t<decaltype(boundTask)()>;
			using PackagedTask = std::packaged_task<ResultType()>;
			using TaskType = ThreadTask<PackagedTask>;

			PackagedTask task{ std::move(boundTask) };
			TaskFuture<ResultType> result{ task.get_future() };
			myWorkQueue.Push(std::make_unique<TaskType>(std::move(task)));
			return result;
		}

	private:
		void Worker(void)
		{
			while (!myDone)
			{
				std::unique_ptr<IThreadTask> pTask{ nullptr };
				if (myWorkQueue.WaitPop(pTask))
				{
					pTask->Execute();
				}
			}
		}

		void Destroy(void)
		{
			myDone = true;
			myWorkQueue.Invalidate();
			for (auto& thread : myThreads)
			{
				if (thread.joinable())
				{
					thread.join();
				}
			}
		}

	private:
		std::atomic_bool myDone;
		CommonUtilities::ThreadSafeQueue<std::unique_ptr<IThreadTask>> myWorkQueue;
		std::vector<std::thread> myThreads;
	};

	namespace DefaultThreadPool
	{
		inline ThreadPool& GetThreadPool(void)
		{
			static ThreadPool defaultPool;
			return defaultPool;
		}

		template <typename Func, typename... Args>
		inline auto SubmitJob(Func&& func, Args&&... args)
		{
			return GetThreadPool().Submit(std::forward<Func>(func), std::forward<Args>(args)...);
		}
	}
}

#endif
