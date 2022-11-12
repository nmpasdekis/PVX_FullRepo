#pragma once
#include <string>
#include <cstdint>

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
		bool Connect();
		void Disconnect();
		size_t Read(uint8_t* Buffer, uint32_t Size);
		size_t Read2(uint8_t* Buffer, uint32_t Size);
		size_t Write(uint8_t * Buffer, uint32_t Size);
		size_t AvailableInBytes();
	};
}

#elif __linux

#endif