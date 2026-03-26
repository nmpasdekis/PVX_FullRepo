#include<vector>
#include<mutex>
#include<condition_variable>
#include<V8.h>
#include <libplatform/libplatform.h>


std::unique_ptr<v8::Platform> platform;

struct _V8_Initializer {
    _V8_Initializer() {
        v8::V8::InitializeICUDefaultLocation(nullptr);
        v8::V8::InitializeExternalStartupData(nullptr);

        platform = v8::platform::NewSingleThreadedDefaultPlatform();
        v8::V8::InitializePlatform(platform.get());
        v8::V8::Initialize();
    }
    ~_V8_Initializer() {
        v8::V8::Dispose();
        platform.reset();
    }
} _V8_Initializer_;

struct IsolateNode {
    v8::ArrayBuffer::Allocator* allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate* isolate = [this] {
        v8::Isolate::CreateParams params;
        params.array_buffer_allocator = allocator;
        return v8::Isolate::New(params);
    }();
    IsolateNode* next = nullptr;

    ~IsolateNode() {
        isolate->Dispose();
        delete allocator;
    }
};

std::mutex IsolateAcquisitionMutex;
std::condition_variable WaitForIsolate;
std::vector<IsolateNode> Isolates;
IsolateNode* NextIsolate;

void InitV8(int IsolateCount) {
    Isolates.resize(IsolateCount);
    for (auto i = 1; i < Isolates.size(); i++) {
        Isolates[i - 1].next = &Isolates[i];
    }
    Isolates.back().next = nullptr;
    NextIsolate = &Isolates[0];
}

IsolateNode * Acquire() {
    std::unique_lock<std::mutex> lock{ IsolateAcquisitionMutex };
    WaitForIsolate.wait(lock, [] { return NextIsolate != nullptr; });
    auto ret = NextIsolate;
    NextIsolate = NextIsolate->next;
    return ret;
}

void Release(IsolateNode* iso) {
    {
        std::lock_guard<std::mutex> lock{ IsolateAcquisitionMutex };
        iso->next = NextIsolate;
        NextIsolate = iso;
    }
    WaitForIsolate.notify_one();
}

