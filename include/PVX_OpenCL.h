#pragma once

#define CL_USE_DEPRECATED_OPENCL_2_0_APIS

#include <CL/cl.hpp>
#include <vector>
#include <map>
#include <variant>

namespace PVX {
	class OpenCL;

	enum class BufferAccess : unsigned int {
		GPU_00_CPU_00 = 0,
		GPU_0W_CPU_00 = CL_MEM_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS,
		GPU_R0_CPU_00 = CL_MEM_READ_ONLY  | CL_MEM_HOST_NO_ACCESS,
		GPU_RW_CPU_00 = CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,

		GPU_0W_CPU_R0 = CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
		GPU_R0_CPU_R0 = CL_MEM_READ_ONLY  | CL_MEM_HOST_READ_ONLY,
		GPU_RW_CPU_R0 = CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,

		GPU_0W_CPU_0W = CL_MEM_WRITE_ONLY | CL_MEM_HOST_WRITE_ONLY,
		GPU_R0_CPU_0W = CL_MEM_READ_ONLY  | CL_MEM_HOST_WRITE_ONLY,
		GPU_RW_CPU_0W = CL_MEM_READ_WRITE | CL_MEM_HOST_WRITE_ONLY,

		zCOPY_HOST_PTR = CL_MEM_COPY_HOST_PTR,
		zCOPY_HOST_PTR_GPU_0W_CPU_00 = CL_MEM_COPY_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS,
		zCOPY_HOST_PTR_GPU_R0_CPU_00 = CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY  | CL_MEM_HOST_NO_ACCESS,
		zCOPY_HOST_PTR_GPU_RW_CPU_00 = CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
		zCOPY_HOST_PTR_GPU_0W_CPU_R0 = CL_MEM_COPY_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
		zCOPY_HOST_PTR_GPU_R0_CPU_R0 = CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY  | CL_MEM_HOST_READ_ONLY,
		zCOPY_HOST_PTR_GPU_RW_CPU_R0 = CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
		zCOPY_HOST_PTR_GPU_0W_CPU_0W = CL_MEM_COPY_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_WRITE_ONLY,
		zCOPY_HOST_PTR_GPU_R0_CPU_0W = CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY  | CL_MEM_HOST_WRITE_ONLY,
		zCOPY_HOST_PTR_GPU_RW_CPU_0W = CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_WRITE_ONLY,
	};

	class Buffer {
	public:
		//Buffer(const int size=0);
		int Read(void* data);
		int ReadBlocking(void* data);
		int Read(void* data, size_t Size);
		int ReadBlocking(void* data, size_t Size);
		int Read(void* data, const std::vector<cl::Event>* WaitFor, cl::Event* ev);
		int ReadBlocking(void* data, const std::vector<cl::Event>* WaitFor, cl::Event* ev);
		int Read(void* data, size_t Size, const std::vector<cl::Event>* WaitFor, cl::Event* ev);
		int ReadBlocking(void* data, size_t Size, const std::vector<cl::Event>* WaitFor, cl::Event* ev);
		template<typename T>
		std::vector<T> Read() {
			std::vector<T> ret(size/sizeof(T));
			Read(&ret[0]);
			return ret;
		}
		template<typename T>
		std::vector<T> Read(size_t Size) {
			std::vector<T> ret(Size);
			Read(&ret[0], Size * sizeof(T));
			return std::move(ret);
		}
		template<typename T>
		std::vector<T> ReadBlocking() {
			std::vector<T> ret(size/sizeof(T));
			auto dbg = ReadBlocking(&ret[0]);
			return std::move(ret);
		}
		template<typename T>
		std::vector<T> ReadBlocking(size_t Size) {
			std::vector<T> ret(Size);
			ReadBlocking(&ret[0], Size * sizeof(T));
			return std::move(ret);
		}

		template<typename T>
		std::vector<T> Read(const std::vector<cl::Event>* WaitFor, cl::Event* ev) {
			std::vector<T> ret(size/sizeof(T));
			Read(&ret[0], WaitFor, ev);
			return ret;
		}
		template<typename T>
		std::vector<T> Read(size_t Size, const std::vector<cl::Event>* WaitFor, cl::Event* ev) {
			std::vector<T> ret(Size);
			Read(&ret[0], Size * sizeof(T), WaitFor, ev);
			return std::move(ret);
		}
		template<typename T>
		std::vector<T> ReadBlocking(const std::vector<cl::Event>* WaitFor, cl::Event* ev) {
			std::vector<T> ret(size/sizeof(T));
			ReadBlocking(&ret[0], WaitFor, ev);
			return std::move(ret);
		}
		template<typename T>
		std::vector<T> ReadBlocking(size_t Size, const std::vector<cl::Event>* WaitFor, cl::Event* ev) {
			std::vector<T> ret(Size);
			ReadBlocking(&ret[0], Size * sizeof(T), WaitFor, ev);
			return std::move(ret);
		}

		int Write(void* data, size_t Size = 0, size_t Offset = 0);
		inline int Size() const { return size; }
		inline BufferAccess GetAccess() const { return Access; }

		const cl::Buffer& operator()() const { return buffer; };
	private:
		friend class OpenCL;
		friend class Kernel;
		int size;
		BufferAccess Access;
		OpenCL* Parent;
		cl::Buffer buffer;
		Buffer(int Size, const cl::Buffer& b, OpenCL* p, BufferAccess Access);
		Buffer(OpenCL* p);
	};

	class LocalBuffer {
		size_t Size;
	public:
		explicit LocalBuffer(size_t Size) : Size{ Size } {}
		inline size_t Get() const { return Size; }
	};

	class KernelParam {
		std::variant<cl::Buffer, LocalBuffer, int, float, size_t> Value;
	public:
		KernelParam(const Buffer& buf) : 
			Value{ buf() } {
		}
		KernelParam(const cl::Buffer& buf) :
			Value{ buf } {}
		template<typename T>
		KernelParam(const T& value) : Value{ value } {
		};
		void Apply(cl_uint Index, cl::Kernel& k) const {
			switch (Value.index()) {
				case 0: k.setArg(Index, std::get<0>(Value)); break;
				case 1: k.setArg(Index, std::get<1>(Value).Get(), nullptr); break;
				case 2: k.setArg(Index, std::get<2>(Value)); break;
				case 3: k.setArg(Index, std::get<3>(Value)); break;
				case 4: k.setArg(Index, std::get<4>(Value)); break;
				//case 5: k.setArg(Index, std::get<5>(Value)); break;
			}
		}
	};

	class Kernel {
	public:
		Kernel(const cl::Kernel& k, OpenCL* Parent);
		int Run(const cl::NDRange& Global, const std::vector<KernelParam>& Params);
		int Run(const cl::NDRange& Global, const cl::NDRange& Local, const std::vector<KernelParam>& Params);

		int Run(const cl::NDRange& Global, const cl::NDRange& Local, const std::vector<KernelParam>& Params, 
			const std::vector<cl::Event>* WaitFor, cl::Event* ev);
		cl::Kernel kernel;
		int WorkGroupSize();
	private:
		OpenCL* Parent;
	};

	class Device {
	public:
		cl::Device device;
		std::string Name;
	};

	class Platform {
	public:
		cl::Platform platform;
		std::string Name;
		std::vector<Device> Devices;
	};

	class OpenCL {
	public:
		OpenCL(int GPU = 1, int skip = 0);

		int LoadProgram(const std::string& Source);
		Buffer MakeBuffer(size_t ByteSize, BufferAccess Access, const void* Data = 0);
		template<typename T>
		Buffer MakeBuffer(const std::vector<T>& Data, BufferAccess Access = BufferAccess::GPU_RW_CPU_00) {
			return MakeBuffer(Data.size() * sizeof(T), Access, Data.data());
		}
		Buffer MakeBuffer();
		Kernel* GetKernel(const std::string& Name);
		cl::CommandQueue& Queue();
		static std::vector<Platform> Get(int GPU = 1);

		inline cl::Device GetDevice() { return device; };
		inline cl::Context GetContext() { return context; };
		std::string PlatformName;
		std::string DeviceName;

	private:
		struct KernelData {
			cl::Program* program;
			Kernel* kernel;
		};
		friend class Kernel;
		friend class Buffer;
		std::map<std::string, KernelData> KernelMap;
		cl::Platform platform;
		cl::Device device;
		cl::Context context;
		std::vector<cl::Program> Programs;
		std::vector<cl::CommandQueue> Queues;
		cl::CommandQueue* currentQueue;
	};
	inline BufferAccess operator|(BufferAccess a, BufferAccess b) {
		return BufferAccess(unsigned int(a) | unsigned int(b));
	}
	inline BufferAccess operator&(BufferAccess a, BufferAccess b) {
		return BufferAccess(unsigned int(a) & unsigned int(b));
	}
	inline BufferAccess& operator|=(BufferAccess& a, const BufferAccess b) {
		return *(BufferAccess*)&((*(unsigned int*)&a) |= unsigned int(b));
	}
}
