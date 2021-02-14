#include <PVX_OpenCL.h>


namespace PVX {
	OpenCL::OpenCL(int GPU, int skip) {
		std::vector<cl::Platform> pl;
		cl::Platform::get(&pl);
		for (auto & p : pl) {
			std::vector<cl::Device> devs;
			p.getDevices(GPU ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_ALL, &devs);
			if (devs.size() && !skip--) {
				platform = p;
				device = devs.front();
				context = cl::Context(device);
				Queues.push_back(cl::CommandQueue(context, device));
				currentQueue = &Queues.back();
				DeviceName = device.getInfo<CL_DEVICE_NAME>();
				PlatformName = platform.getInfo<CL_PLATFORM_NAME>();
				return;
			}
		}
		throw "Device Not Found";
	}

	std::vector<Platform> OpenCL::Get(int GPU) {
		std::vector<Platform> ret;

		std::vector<cl::Platform> pl;
		cl::Platform::get(&pl);
		for (auto& p : pl) {
			auto& pp = ret.emplace_back();
			pp.platform = p;
			pp.Name = p.getInfo<CL_PLATFORM_NAME>();
			std::vector<cl::Device> devs;
			p.getDevices(GPU ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_ALL, &devs);
			for (auto& d : devs) {
				auto& dd = pp.Devices.emplace_back();
				dd.device = d;
				dd.Name = d.getInfo<CL_DEVICE_NAME>();
			}
		}
		return ret;
	}

#define IS_SPACE(x) ((x)==' '||(x)=='\t'||(x)=='\r'||(x)=='\n')

#ifdef _DEBUG
#define BUILD_OPTIONS
#else
#define BUILD_OPTIONS
	// "-g -O0"
#endif

	int OpenCL::LoadProgram(const std::string & Source) {
		const char * s = Source.c_str();
		int start = 0;
		int tmp = 0;
		std::vector<std::string> Names;
		while ((-1 != (tmp = Source.find("__kernel", start))) || (-1 != (tmp = Source.find("kernel", start)))) {
			start = tmp + ((s[tmp] == '_') ? 8 : 6);
			if (!(tmp == 0 || IS_SPACE(s[tmp - 1])) && (IS_SPACE(s[start]))) continue;

			int end = Source.find('(', start);
			if (end == -1) return 0;
			while (IS_SPACE(s[end - 1])) end--;
			start = end - 1;
			while (!IS_SPACE(s[start]))start--;
			start++;
			Names.push_back(Source.substr(start, end - start));
		}
		if (!Names.size())return 0;
		std::string Errors;
		cl::Program::Sources src(1, std::make_pair(Source.c_str(), Source.length() + 1));
		cl::Program prog(context, src);
		if (!prog.build(BUILD_OPTIONS)) {
			Programs.push_back(prog);
			auto * p = &Programs.back();
			for (auto & n : Names) {
				KernelMap[n] = KernelData{ p, 0 };
			}
			return Names.size();
		}
		Errors = prog.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
		throw Errors;
	}

	Buffer OpenCL::MakeBuffer(size_t size, BufferAccess Access, const void * Data) {
		auto acc = Access;
		if (Data)acc |= BufferAccess::zCOPY_HOST_PTR;
		return Buffer(size, std::move(cl::Buffer(context, (int)acc, size, (void*)Data)), this, Access);
	}
	//Buffer OpenCL::MakeBuffer(const std::vector<float>& Data, BufferAccess Access) {
	//	return MakeBuffer(Data.size() * sizeof(float), Access, Data.data());
	//}

	Buffer OpenCL::MakeBuffer() {
		return Buffer(this);
	}

	Kernel * PVX::OpenCL::GetKernel(const std::string & Name) {
		auto k = KernelMap.find(Name);
		if (k != KernelMap.end()) {
			if (!k->second.kernel)
				k->second.kernel = new Kernel(cl::Kernel(*k->second.program, Name.c_str()), this);
			return k->second.kernel;
		}
		return nullptr;
	}
	cl::CommandQueue & OpenCL::Queue() {
		return *currentQueue;
	}
	Kernel::Kernel(const cl::Kernel & k, OpenCL * p):kernel(k), Parent(p) { }
	int Kernel::Run(const cl::NDRange & r, const std::vector<KernelParam>& Params) {
		size_t Index = 0;
		for (auto& p: Params) p.Apply(Index++, kernel);
		return Parent->currentQueue->enqueueNDRangeKernel(kernel, cl::NullRange, r);
	}
	int Kernel::Run(const cl::NDRange & Global, const cl::NDRange & Local, const std::vector<KernelParam>& Params) {
		size_t Index = 0;
		for (auto& p: Params) p.Apply(Index++, kernel);
		return Parent->currentQueue->enqueueNDRangeKernel(kernel, cl::NullRange, Global, Local);
	}
	int Kernel::Run(const cl::NDRange& Global, const cl::NDRange& Local, const std::vector<KernelParam>& Params, const std::vector<cl::Event>* WaitFor, cl::Event* ev) {
		size_t Index = 0;
		for (auto& p: Params) p.Apply(Index++, kernel);
		return Parent->currentQueue->enqueueNDRangeKernel(kernel, cl::NullRange, Global, Local, WaitFor, ev);
	}
	int Kernel::WorkGroupSize() {
		return kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(Parent->device);
	}

	Buffer::Buffer(OpenCL* p) : Parent(p), size{ 0 }, Access{ BufferAccess::GPU_00_CPU_00 } { }
	Buffer::Buffer(int Size, const cl::Buffer& b, OpenCL* p, BufferAccess Access) : size(Size), buffer(b), Parent(p), Access{ Access } { }

	int Buffer::Read(void * data) {
		return Parent->currentQueue->enqueueReadBuffer(buffer, 0, 0, size, data);
	}
	int Buffer::ReadBlocking(void* data) {
		return Parent->currentQueue->enqueueReadBuffer(buffer, true, 0, size, data);
	}
	int Buffer::Read(void* data, size_t Size) {
		return Parent->currentQueue->enqueueReadBuffer(buffer, 0, 0, Size, data);
	}
	int Buffer::ReadBlocking(void* data, size_t Size) {
		return Parent->currentQueue->enqueueReadBuffer(buffer, true, 0, Size, data);
	}

	int Buffer::Read(void* data, size_t Size, const std::vector<cl::Event>* WaitFor, cl::Event* ev) {
		return Parent->currentQueue->enqueueReadBuffer(buffer, 0, 0, Size, data, WaitFor, ev);
	}
	int Buffer::ReadBlocking(void* data, size_t Size, const std::vector<cl::Event>* WaitFor, cl::Event* ev) {
		return Parent->currentQueue->enqueueReadBuffer(buffer, true, 0, Size, data, WaitFor, ev);
	}
	int Buffer::Read(void* data, const std::vector<cl::Event>* WaitFor, cl::Event* ev) {
		return Parent->currentQueue->enqueueReadBuffer(buffer, 0, 0, size, data, WaitFor, ev);
	}
	int Buffer::ReadBlocking(void* data, const std::vector<cl::Event>* WaitFor, cl::Event* ev) {
		return Parent->currentQueue->enqueueReadBuffer(buffer, true, 0, size, data, WaitFor, ev);
	}

	int Buffer::Write(void * data, size_t Size, size_t Offset) {
		if(!Size) Size = this->size;
		return Parent->currentQueue->enqueueWriteBuffer(buffer, 0, Offset, Size, data);
	}
}