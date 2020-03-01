#include<PVX_Threading.h>
#include<mutex>
#include<condition_variable>
#include<atomic>
#include<Windows.h>

namespace PVX {
	namespace Threading {

		struct BarrierPrivate {
			BarrierPrivate();
			std::mutex Mutex;
			std::condition_variable cv;
			std::atomic_int Count;
		};
		BarrierPrivate::BarrierPrivate():Count{0} {}

		Barrier::~Barrier() {
			delete (BarrierPrivate*)PrivateData;
		}

		Barrier::Barrier(std::size_t count) : 
			PrivateData{ new BarrierPrivate() } {}
		
		void Barrier::Wait() {
			BarrierPrivate & pd = *(BarrierPrivate*)PrivateData;
			std::unique_lock<std::mutex> lock{ pd.Mutex };
			if (--pd.Count == 0) {
				pd.cv.notify_all();
			} else {
				pd.cv.wait(lock, [&pd] { return pd.Count == 0; });
			}
		}

		struct TaskPumpPrivate {
			std::mutex Mutex, BarrierMutex;
			std::atomic_bool Running;
			std::atomic_int TaskCount, Working;
			std::condition_variable TaskAdded, TaskRemoved, Barrier;
			std::vector<std::thread> Threads;
		};

		void TaskPump::Worker(int Id) {
			TaskPumpPrivate & pd = *(TaskPumpPrivate*)PrivateData;
			for (;;) {
				std::function<void()> NextTask;
				{
					std::unique_lock<std::mutex> lock{ pd.Mutex };
					pd.TaskAdded.wait(lock, [this, &pd] { return pd.TaskCount > 0; });
					if (!pd.Running)return;
					NextTask = Tasks.front();
					Tasks.pop();
					pd.TaskCount--;
				}
				pd.Working++;
				NextTask();
				pd.Working--;
				pd.Barrier.notify_one();
				pd.TaskRemoved.notify_one();
			}
		}

		TaskPump::TaskPump(int Count) : PrivateData{ new TaskPumpPrivate() } {
			TaskPumpPrivate & pd = *(TaskPumpPrivate*)PrivateData;
			pd.Running = true;
			if (Count <= 0)
				Count = std::thread::hardware_concurrency() - 1;
			for (int i = 0; i < Count; i++) {
				pd.Threads.push_back(std::thread(&TaskPump::Worker, this, i));
			}
		}
		TaskPump::~TaskPump() {
			TaskPumpPrivate * pd = (TaskPumpPrivate*)PrivateData;
			if (pd->Running)Release();
			delete pd;
		}
		void TaskPump::Enqueue(std::function<void()> Task, int Limit) {
			TaskPumpPrivate & pd = *(TaskPumpPrivate*)PrivateData;
			{
				std::unique_lock<std::mutex> lock{ pd.Mutex };
				if (Limit) pd.TaskRemoved.wait(lock, [this, Limit, &pd] { return pd.TaskCount < Limit; });
				Tasks.push(Task);
				pd.TaskCount++;
			}
			pd.TaskAdded.notify_one();
		}
		void TaskPump::Release() {
			TaskPumpPrivate & pd = *(TaskPumpPrivate*)PrivateData;
			pd.Running = false;
			pd.TaskCount = 1;
			pd.TaskAdded.notify_all();
			for (auto & t : pd.Threads)
				t.join();
		}
		void TaskPump::Wait() {
			TaskPumpPrivate & pd = *(TaskPumpPrivate*)PrivateData;
			std::unique_lock<std::mutex> lock{ pd.BarrierMutex };
			pd.Barrier.wait(lock, [this, &pd] {
				return pd.TaskCount == 0 && pd.Working == 0;
			});
		}

		struct UnreliableParallelPrivate {
			std::mutex Mutex;
			std::thread Thread;
			std::condition_variable cv;
			std::atomic_bool Running;
		};

		UnreliableParallel::UnreliableParallel() : PrivateData(new UnreliableParallelPrivate()) {
			UnreliableParallelPrivate & pd = *(UnreliableParallelPrivate*)PrivateData;
			Ratio = 0.5f;
			RatioRate = 0.005f;
			pd.Running = true;
			pd.Thread = std::thread([this, &pd] {
				for (;;) {
					std::unique_lock<std::mutex> Lock{ pd.Mutex };
					pd.cv.wait(Lock, [this, &pd] { return ThreadProc != nullptr || !pd.Running; });
					if (pd.Running) {
						ThreadProc();
						ThreadProc = nullptr;
						continue;
					}
					return;
				}
			});
		}
		UnreliableParallel::~UnreliableParallel() {
			UnreliableParallelPrivate & pd = *(UnreliableParallelPrivate*)PrivateData;
			pd.Running = false;
			pd.cv.notify_one();
			pd.Thread.join();
			//std::unique_lock<std::mutex> Lock{ pd.Mutex };
			delete (UnreliableParallelPrivate*)PrivateData;
		}
		bool UnreliableParallel::TryExecute(std::function<void()> clb) {
			UnreliableParallelPrivate & pd = *(UnreliableParallelPrivate*)PrivateData;
			Ratio *= 1.0f - RatioRate;
			if(pd.Mutex.try_lock()) {
			//if (std::try_lock((pd.Mutex))) {
				ThreadProc = clb;
				pd.cv.notify_one();
				Ratio+=RatioRate;
				pd.Mutex.unlock();
				return true;
			}
			return false;
		}

		//struct ThreadData {
		//	std::function<void()> func;
		//};

		//static DWORD WINAPI ThreadCallback(_In_ LPVOID data) {
		//	ThreadData * dt = (ThreadData*)data;
		//	dt->func();
		//	delete dt;
		//	return 0;
		//}

		//Thread::Thread(std::function<void()> fnc) {
		//	ThreadData * dt = new ThreadData();
		//	dt->func = fnc;
		//	th = CreateThread(NULL, 0, ThreadCallback, dt, 0, 0);
		//}
		//void Thread::Join() {
		//	WaitForSingleObject(th, INFINITE);
		//}

		struct TimeoutPrivateData {
			std::mutex Mutex;
			std::function<void()> func, thFunc;
			std::thread th;
			int Running, Remaining;
			int OriginalTimer, chechIntervalInMillisec;
		};

		Timeout::Timeout(std::function<void()> callback, int DelayInMillisec, int chechIntervalInMillisec) {
			TimeoutPrivateData & pd = *(new TimeoutPrivateData());
			PrivateData = &pd;
			pd.chechIntervalInMillisec = chechIntervalInMillisec;
			pd.func = callback;
			pd.Remaining = DelayInMillisec;
			pd.OriginalTimer = DelayInMillisec;
			pd.Running = 1;
			pd.thFunc = [&pd] {
				while (pd.Running && pd.Remaining) {
					std::unique_lock<std::mutex> lock{ pd.Mutex };
					std::this_thread::sleep_for(std::chrono::milliseconds(pd.chechIntervalInMillisec));
					pd.Remaining -= pd.chechIntervalInMillisec;
				}
				{
					std::unique_lock<std::mutex> lock{ pd.Mutex };
					if (pd.Running) {
						pd.func();
						pd.Running = 0;
					}
				}
			};
			pd.th = std::thread(pd.thFunc);
		}
		Timeout::~Timeout() {
			Cancel();
			delete (TimeoutPrivateData*)PrivateData;
		}
		void Timeout::Cancel() {
			TimeoutPrivateData & pd = *(TimeoutPrivateData*)PrivateData;
			{
				std::unique_lock<std::mutex> lock{ pd.Mutex };
				pd.Running = 0;
			}
			pd.th.join();
		}
		void Timeout::Reset() {
			TimeoutPrivateData & pd = *(TimeoutPrivateData*)PrivateData; 
			{
				std::unique_lock<std::mutex> lock{ pd.Mutex };
				pd.Remaining = pd.OriginalTimer;
			}
			if (!pd.Running) {
				pd.th.join();
				pd.Running = 1;
				pd.th = std::thread(pd.thFunc);
			}
		}
}
}