/**
 * @file WorkerThread.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The worker thread class.
 *
 * A thread that is used in a threads pool, executing jobs.
 */

#pragma once

#ifndef __WORKER_THREAD_H_
#define __WORKER_THREAD_H_

#include <thread>
#include <atomic>
#include <mutex>


namespace Utils {

	/**
	 * @class WorkerThread
	 * @brief WorkerThread class for a thread in a threads pool.
	 *
	 * A thread that is used in a threads pool, executing jobs.
	 * 
	 * @tparam ThreadsPool The threads pool class.
	 * @tparam Job The job class/type.
	 * @sa ThreadsPool
	 */
	template<class ThreadsPool, class Job> class WorkerThread
	{
	public:
		/**
		 * @brief Construct a new Worker Thread object.
		 *
		 * Constructs a new Worker Thread object, associated with the given threads pool.
		 * The thread is started immediately.
		 * 
		 * @param threadsPool The threads pool that this thread belongs to.
		 */
		explicit WorkerThread(ThreadsPool* threadsPool)
			: threadsPool(threadsPool), StopFlag(false)
		{
			Thread = std::thread(&WorkerThread::Run, this);
		}

		/**
		 * @brief Destroy the Worker Thread object.
		 *
		 * Destroys the Worker Thread object, stopping the thread if it is still running.
		 */
		~WorkerThread()
		{
			Stop();
		}

		/**
		 * @brief Start the thread.
		 *
		 * Starts the thread if not already started.
		 */
		void Start()
		{
			if (Thread.joinable()) return;

			{
				std::lock_guard lock(threadsPool->Mutex);
				StopFlag = false;
			}
			Thread = std::thread(&WorkerThread::Run, this);
		}

		/**
		 * @brief Stop the thread.
		 *
		 * Stops the thread if it is running, and waits for it to finish.
		 */
		void Stop()
		{
			if (!Thread.joinable()) return;

			{
				std::lock_guard lock(threadsPool->Mutex);

				StopFlag = true;
			}

			Join();
		}

		/**
		 * @brief Set the stop flag without locking it.
		 * 
		 * Sets the stop flag to true without locking the threads pool mutex.
		 */
		void SetStopUnlocked()
		{
			StopFlag = true;
		}

		/**
		 * @brief Join the thread.
		 *
		 * Joins the thread if it is joinable, waiting for it to finish.
		 */
		inline void Join()
		{
			if (Thread.joinable()) Thread.join();
		}

	private:
		/**
		 * @brief Check if the thread should terminate waiting.
		 * 
		 * Checks if the thread should terminate waiting for new jobs.
		 * The thread should terminate waiting if there is work to do or if the stop flag is set.
		 * 
		 * @return true if the thread should terminate waiting, false otherwise.
		 */
		inline bool TerminateWait() const
		{
			return threadsPool->HasWork() || StopFlag;
		}

		/**
		 * @brief The main function of the thread.
		 *
		 * The main function of the thread, which runs in a loop, waiting for jobs to execute.
		 * When a job is available, it is executed, and the thread goes back to waiting for more jobs.
		 * If the stop flag is set, the thread exits the loop and terminates.
		 */
		void Run()
		{
			for (;;)
			{
				std::unique_lock lock(threadsPool->Mutex);
				if (!TerminateWait())
					threadsPool->Condition.wait(lock, [this] { return TerminateWait(); });

				while (threadsPool->HasWork() && !StopFlag)
				{
					const std::shared_ptr<Job> job = std::move(threadsPool->JobsQueue.front());
					threadsPool->JobsQueue.pop();
					lock.unlock();

					job->DoWork();

					{
						std::lock_guard lockCount(threadsPool->FinishMutex);
						threadsPool->FinishCount += job->GetJobCount();
					}
					lock.lock();
				}

				if (StopFlag) break;
				lock.unlock();
				threadsPool->NotifyFinish();
			}
		}

		ThreadsPool* threadsPool; /**< The threads pool that this thread belongs to. */
		std::thread Thread; /**< The thread object. */
		bool StopFlag; /**< The stop flag, indicating if the thread should terminate. */
	};


}

#endif // !__WORKER_THREAD_H_

