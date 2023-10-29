#define NOMINMAX

#include <PVX_ComPort.h>
#include <algorithm>

namespace PVX::Serial {
	Com::Com(uint8_t Num, uint32_t BaudRate) {
		commName = [](uint8_t n) {
			using namespace std::string_literals;
			if(n<=9) return "COM"s + std::to_string(n);
			return "\\\\.\\COM"s + std::to_string(n);
		}(Num);
		baudRate = BaudRate;
		//Connect();
	}

	uint32_t Com::Connect() {
		hCom = CreateFileA(static_cast<LPCSTR>(commName.c_str()), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		Error = GetLastError();
		if (!Error) {
			DCB params{};
			if (GetCommState(hCom, &params)) {
				params.BaudRate = baudRate;
				params.ByteSize = 8;
				params.StopBits = ONESTOPBIT;
				params.Parity = NOPARITY;
				params.fDtrControl = DTR_CONTROL_ENABLE;
				if (SetCommState(hCom, &params)) {
					PurgeComm(hCom, PURGE_RXCLEAR | PURGE_TXCLEAR);
				}
			}
		}
		return Error;
	}

	void Com::Disconnect() {
		if (hCom) CloseHandle(hCom);
		hCom = NULL;
	}

	size_t Com::Read(uint8_t* Buffer, uint32_t Size) {
		DWORD ret = 0;

		if (ReadFile(hCom, Buffer, Size, &ret, NULL)) {
			return ret;
		} else {
			Error = GetLastError();
			return 0;
		}
		return 0;
	}
	size_t Com::Read2(uint8_t* Buffer, uint32_t Size) {
		DWORD ret = 0;
		ClearCommError(hCom, &Error, &status);
		if (Error)
			return 0;
		Size = std::min(Size, (uint32_t)status.cbInQue);
		//if (Size)
		if (ReadFile(hCom, Buffer, Size, &ret, NULL)) {
			return ret;
		} else {
			Error = GetLastError();
			return 0;
		}
		return 0;
	}
	size_t Com::Write(uint8_t* Buffer, uint32_t Size) {
		ClearCommError(hCom, &Error, &status);
		if (Error)
			return 0;
		DWORD writen = 0;
		if (WriteFile(hCom, Buffer, Size, &writen, NULL)) {
			if (Size != writen)
				Error = 1;
			return writen;
		} else {
			Error = GetLastError();
			return 0;
		}
		return 0;
	}
	size_t Com::AvailableInBytes() {
		ClearCommError(hCom, &Error, &status);
		if (Error) {
			Error = GetLastError();
			return 0;
		}
		return status.cbInQue;
	}
}