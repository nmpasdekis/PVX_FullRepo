#pragma once
#include <string>
#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>
#include <memory>
#include <deque>

#ifdef _WIN32
#include <Windows.h>
namespace PVX::Serial {

	class Com {
		std::string commName;
		uint32_t baudRate;
		HANDLE hCom{};
		COMSTAT status{};
		DWORD Error{};
	public:
		Com(uint8_t Num, uint32_t BaudRate = 9600);
		~Com() { Disconnect(); }
		uint32_t Connect();
		void Disconnect();
		size_t Read(uint8_t* Buffer, uint32_t Size);

		template<typename T>
		T Read() {
			T ret{};
			Read((uint8_t*)&ret, sizeof(T));
			return ret;
		}
		template<typename T>
		size_t Read(T& ret) {
			return Read((uint8_t*)&ret, sizeof(T));
		}

		std::string ReadString() {
			std::string ret;
			auto sz = AvailableInBytes();
			if (sz) {
				ret.resize(sz);
				auto sz = Read((uint8_t*)ret.data(), ret.size());
			}
			return ret;
		};
		int WriteString(const std::string& str) {
			return Write((uint8_t*)str.data(), str.size());
		}

		size_t Read2(uint8_t* Buffer, uint32_t Size);
		size_t Write(uint8_t * Buffer, uint32_t Size);
		size_t AvailableInBytes();
	};

	class ComEvent {
		std::string commName;
		uint32_t baudRate;
		HANDLE hCom{};
		COMSTAT status{};
		DWORD Error{};
		std::atomic_bool Running;

		std::vector<uint8_t> Buffer;
		std::deque< std::vector<uint8_t>> SendQueue;
		std::mutex Mutex;

		std::unique_ptr<std::thread> runThread{};

		void threadMember();
		
	public:
		ComEvent(uint8_t Num, uint32_t BaudRate = 9600);
		~ComEvent() { Disconnect(); }
		uint32_t Connect();
		void Disconnect();

		std::function<void(ComEvent&, const std::vector<uint8_t>&)> OnReceive;
		std::function<void(ComEvent&, int)> OnError;

		void Send(const void* Buffer, size_t Size);
		inline void Send(const std::vector<uint8_t>& data) {
			Send(data.data(), data.size());
		}
		inline void Send(const std::string& data) {
			Send(data.data(), data.size());
		}
	};
}

#elif __linux

#endif