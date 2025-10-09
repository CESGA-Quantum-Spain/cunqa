/**
 * @file ThreadsPool.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The pool of threads, executing jobs.
 *
 * A thread pool with reusable threads, executing jobs.
 */

#pragma once

#ifndef __THREADS_POOL_H_
#define __THREADS_POOL_H_

#include <vector>
#include <queue>
#include <condition_variable>

#include <fstream>

#include "WorkerThread.h"

namespace Utils {

	/**
	 * @class ThreadsPool
	 * @brief ThreadsPool class for holding and controlling a pool of threads.
	 *
	 * Contains a bunch of threads that can execute jobs, supplying functions to add jobs, wait for their completion, start and stop the threads.
	 * Allows adding operations and converting them to prepare the circuit for distributed computing.
	 * @tparam Job The job class/type.
	 * @sa WorkerThread
	 */
	template<class Job> class ThreadsPool
	{
		using JobWorkerThread = WorkerThread<ThreadsPool<Job>, Job>;
		friend class WorkerThread<ThreadsPool<Job>, Job>;
	public:
		/**
		 * @brief Construct a new Thread pool object.
		 *
		 * Constructs a new Thread pool object with the given number of threads.
		 * @param nrThreads The number of threads to create in the pool. If less than or equal to zero, one thread will be created. Can be resized later.
		 */
		explicit ThreadsPool(int nrThreads = 0)
		{
			if (nrThreads <= 0) nrThreads = 1;

			for (int i = 0; i < nrThreads; ++i)
				Threads.emplace_back(std::make_unique<JobWorkerThread>(this));
		}

		/**
		 * @brief Destructor.
		 *
		 * Destroy the threads pool object.
		 * Before destroying, stops all threads and waits for their completion.
		 */
		~ThreadsPool()
		{
			Stop();
		}


		/**
		 * @brief Stop all the threads in the threads pool.
		 *
		 * Stops all the threads and clears the threads pool.
		 * Waits for all threads to complete before returning.
		 */
		void Stop()
		{
			{
				std::lock_guard lock(Mutex);

				for (auto& worker : Threads)
					worker->SetStopUnlocked();
			}

			NotifyAll();

			for (auto& worker : Threads)
				worker->Join();

			Threads.clear();
		}

		/**
		 * @brief Start all the threads in the threads pool.
		 *
		 * Starts all the threads in the threads pool.
		 */
		void Start()
		{
			for (auto& worker : Threads)
				worker->Start();
		}

		/**
		 * @brief Add a job to be executed by the threads pool.
		 *
		 * Adds a job to the queue of jobs to be executed by the threads pool.
		 * Notifies one thread that there is a new job to execute.
		 * @param job The job to be executed.
		 */
		void AddRunJob(const std::shared_ptr<Job>& job)
		{
			{
				std::lock_guard lock(Mutex);
				JobsQueue.push(job);
			}

			NotifyOne();
		}

		/**
		 * @brief Add a job to be executed by the threads pool.
		 *
		 * Adds a job to the queue of jobs to be executed by the threads pool.
		 * Notifies one thread that there is a new job to execute.
		 * @param job The job to be executed.
		 */
		void AddRunJob(std::shared_ptr<Job>&& job)
		{
			{
				std::lock_guard lock(Mutex);
				JobsQueue.push(std::move(job));
			}

			NotifyOne();
		}

		/**
		 * @brief Wait for all jobs to finish.
		 *
		 * Waits for all jobs to finish executing.
		 * This function will block until the number of finished jobs reaches the finish limit set by SetFinishLimit().
		 * If no finish limit is set, it will wait indefinitely.
		 */
		void WaitForFinish()
		{
			do
			{
				std::unique_lock lock(FinishMutex);

				if (FinishCount >= FinishLimit) return;

				ConditionFinish.wait(lock, [this] { return FinishCount >= FinishLimit; });

				if (FinishCount >= FinishLimit)
					return;
			} while (true);
		}

		/**
		 * @brief Resize the threads pool.
		 *
		 * Resizes the threads pool to the given number of threads.
		 * If the new size is greater than the current size, new threads will be created and started.
		 * If the new size is less than the current size, excess threads will be stopped.
		 * @param nrThreads The new number of threads in the pool. If less than or equal to zero, one thread will be created or remain.
		 */
		void Resize(size_t nrThreads)
		{
			if (nrThreads <= 0) nrThreads = 1;

			size_t oldSize = Threads.size();

			if (oldSize == nrThreads) return;

			if (oldSize < nrThreads)
			{
				for (size_t i = oldSize; i < nrThreads; ++i)
				{
					Threads.emplace_back(std::make_unique<JobWorkerThread>(this));
					Threads.back()->Start();
				}
			}
			else
			{
				{
					std::lock_guard lock(Mutex);
					for (size_t i = oldSize; i < nrThreads; ++i)
						Threads[i]->SetStopUnlocked();
				}

				NotifyAll();

				for (size_t i = oldSize; i < nrThreads; ++i)
					Threads[i]->Join();
				Threads.resize(nrThreads);
			}
		}

		/**
		 * @brief Set the finish limit.
		 *
		 * Sets the finish limit for the threads pool.
		 * The finish limit is the number of jobs that need to be finished before WaitForFinish() returns.
		 * If no finish limit is set, WaitForFinish() will wait indefinitely.
		 * @param limit The finish limit. Default is std::numeric_limits<size_t>::max().
		 */
		void SetFinishLimit(size_t limit)
		{
			std::unique_lock lock(FinishMutex);
			FinishLimit = limit;
			FinishCount = 0;
		}

	private:
		/**
		 * @brief Notify one thread that there is a new job to execute.
		 * 
		 * Notifies a single thread that there is a new job to execute.
		 */
		inline void NotifyOne()
		{
			Condition.notify_one();
		}

		/**
		 * @brief Notify all threads that something new has occured.
		 * 
		 * Notifies all threads that there is a new job to execute or that they need to terminate.
		 */
		inline void NotifyAll()
		{
			Condition.notify_all();
		}

		/**
		 * @brief Check if there are jobs to be executed.
		 * 
		 * Checks if there are jobs in the queue to be executed.
		 * @return true if there are jobs to be executed, false otherwise.
		 */
		inline bool HasWork() const
		{
			return !JobsQueue.empty();
		}

		/**
		 * @brief Notify all waiting threads that the finish condition may have been met.
		 * 
		 * Notifies all threads waiting in WaitForFinish() that the finish condition may have been met.
		 */
		void NotifyFinish()
		{
			ConditionFinish.notify_all();
		}

		std::mutex Mutex; /**< Mutex to be used to check the jobs queue */
		std::condition_variable Condition; /**< Condition variable to notify threads of new jobs */

		std::mutex FinishMutex; /**< Mutex to be used to check the finish condition */
		std::condition_variable ConditionFinish; /**< Condition variable to notify threads waiting for finish */

		size_t FinishCount = 0; /**< The number of finished jobs */
		size_t FinishLimit = std::numeric_limits<size_t>::max(); /**< The limit of the finished jobs to reach before notifying finish */

		std::queue<std::shared_ptr<Job>> JobsQueue; /**< The queue of the jobs to be executed */

		std::vector<std::unique_ptr<JobWorkerThread>> Threads; /**< The vector with the worker threads */
	};

}

#endif // __THREADS_POOL_H_
