#include <PVX_V8.h>
#include <PVX_File.h>

namespace PVX::Javascript {
	AsyncEngine::AsyncEngine() :Looper([this] {
		Engine eng;
		Running = true;
		for(;;) {
			auto lock = std::unique_lock<std::mutex>{ taskMod };
			MainThreadCV.wait(lock, [&] { return Task!=nullptr || !Running; });
			if (!Running) return;
			Task(eng);
			Task = nullptr;
			ThreadCV.notify_one();
		}
	}) {}
	AsyncEngine::~AsyncEngine() {
		Running = false;
		MainThreadCV.notify_all();
		ThreadCV.notify_all();
		Looper.join();
	}
	PVX::JSON::Item AsyncEngine::Do(std::function<JSON::Item(Engine& eng)> task) {
		std::promise<PVX::JSON::Item> prom;
		auto Future = prom.get_future();
		{
			auto lock = std::unique_lock<std::mutex>{ taskMod };
			ThreadCV.wait(lock, [&] { return Task==nullptr || !Running; });
			if (!Running) {
				ThreadCV.notify_all();
				MainThreadCV.notify_all();
				return JSON::jsElementType::Undefined; 
			}
			Task = [&](Engine& eng) {
				prom.set_value(task(eng));
			};
			MainThreadCV.notify_one();
		}
		return Future.get();
	}
	void AsyncEngine::Do_void(std::function<void(Engine& eng)> task) {
		std::promise<void> prom;
		auto Future = prom.get_future();
		{
			auto lock = std::unique_lock<std::mutex>{ taskMod };
			ThreadCV.wait(lock, [&] { return Task==nullptr || !Running; });
			if (!Running) {
				ThreadCV.notify_all();
				MainThreadCV.notify_all();
				return;
			}
			Task = [&](Engine& eng) {
				task(eng);
				prom.set_value();
			};
			MainThreadCV.notify_one();
		}
		Future.get();
	}
	PVX::JSON::Item AsyncEngine::RunCode(const std::wstring& Code) {
		return Do([&](Engine& eng) {
			return eng.RunCode(Code).ToJson();
		});
	}
	void AsyncEngine::RunCode_void(const std::wstring& Code) {
		Do_void([&](Engine& eng) {
			eng.RunCode(Code);
		});
	}
	PVX::JSON::Item AsyncEngine::RunFile(const std::wstring& Filename) {
		if (auto Code = PVX::IO::ReadUtf(Filename);Code.size()) {
			return Do([&](Engine& eng) {
				return eng.RunCode(Code).ToJson();
			});
		}
		return JSON::jsElementType::Undefined;
	}
	void AsyncEngine::RunFile_void(const std::wstring& Filename) {
		if (auto Code = PVX::IO::ReadUtf(Filename);Code.size()) {
			Do_void([&](Engine& eng) {
				eng.RunCode(Code);
			});
		}
	}
	PVX::JSON::Item AsyncEngine::Call(const std::wstring& Function, const std::vector<PVX::JSON::Item>& Params) {
		return Do([&](Engine& eng) { return eng.CallFunction(Function, Params); });
	}
	void AsyncEngine::Call_void(const std::wstring& Function, const std::vector<PVX::JSON::Item>& Params) {
		Do_void([&](Engine& eng) { eng.CallFunction(Function, Params); });
	}
}