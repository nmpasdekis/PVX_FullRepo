#ifndef __PVX_THREADING_H__
#define __PVX_THREADING_H__

#include <functional>
#include <queue>
#include <condition_variable>
#include <mutex>

namespace PVX {
	namespace Threading {
		class Barrier {
		private:
			void * PrivateData;
		public:
			~Barrier();
			explicit Barrier(std::size_t count);
			void Wait();
		};

		class UnreliableParallel {
		private:
			void * PrivateData;
			std::function<void()> ThreadProc;
			float RatioRate;
		public:
			UnreliableParallel();
			~UnreliableParallel();
			bool TryExecute(std::function<void()> clb);
			float Ratio;
		};

		class Timeout {
			void * PrivateData;
		public:
			Timeout(std::function<void()> callback, int DelayInMillisec, int chechIntervalInMillisec=1);
			~Timeout();
			void Cancel();
			void Reset();
		};

		class TaskPump {
		public:
			TaskPump(int Count = 0);
			~TaskPump();
			void Enqueue(std::function<void()> Task, int Limit = 0);
			void Release();
			void Wait();
		private:
			void * PrivateData;
			std::queue<std::function<void()>> Tasks;
			void Worker(int);
		};

		class Pauser {
			std::mutex RauserMutex;
			std::condition_variable cv;
			bool paused = false;
		public:
			inline void Pause() {
				std::unique_lock<std::mutex> lock{ RauserMutex };
				paused = true;
				cv.wait(lock, [this] { return !paused; });
			}
			inline void Unpause() {
				paused = false;
				cv.notify_one();
			}
		};
	}
}

#endif