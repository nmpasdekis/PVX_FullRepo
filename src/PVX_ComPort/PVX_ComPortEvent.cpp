#define NOMINMAX

#include <PVX_ComPort.h>
#include <algorithm>

namespace PVX::Serial {
	using namespace std::chrono_literals;

	ComEvent::ComEvent(uint8_t Num, uint32_t BaudRate) : commName{ commName = [](uint8_t n) {
		using namespace std::string_literals;
		if (n <= 9) return "COM"s + std::to_string(n);
		return "\\\\.\\COM"s + std::to_string(n);
	}(Num) }, baudRate{ BaudRate } {}

	void ComEvent::threadMember() {
		while (Running) {
			ClearCommError(hCom, &Error, &status);
			if (Error) {
				Running = false;
				Disconnect();
				OnError(*this, GetLastError());
				return;
			} else if (status.cbInQue) {
				DWORD ret = 0;
				Buffer.resize(status.cbInQue);
				if (ReadFile(hCom, Buffer.data(), Buffer.size(), &ret, NULL)) {
					OnReceive(*this, Buffer);
				}
				else {
					Running = false;
					Disconnect();
					OnError(*this, GetLastError());
					return;
				}
			}
			else {
				{
					std::lock_guard<std::mutex> lock{ Mutex };
					if (SendQueue.size()) {
						DWORD writen = 0;
						auto dt = std::move(SendQueue.front());
						SendQueue.pop_front();

						if (!WriteFile(hCom, dt.data(), dt.size(), &writen, NULL)) {
							Running = false;
							Disconnect();
							OnError(*this, GetLastError());
							return;
						}
						continue;
					}
				}
				std::this_thread::sleep_for(1us);
			}
		}
	}

	uint32_t ComEvent::Connect() {
		hCom = CreateFileA(static_cast<LPCSTR>(commName.c_str()), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
					Running = true;
					SendQueue.clear();
					runThread.reset(new std::thread(&ComEvent::threadMember, this));
				}
			}
		}
		return Error;
	}

	void ComEvent::Disconnect() {
		Running = false;
		if(runThread)
			runThread->join();
		if (hCom) CloseHandle(hCom);
		hCom = NULL;
	}

	void ComEvent::Send(const void* Buffer, size_t Size) {
		std::lock_guard<std::mutex> lock{ Mutex };
		std::vector<uint8_t> dt(Size);
		memcpy_s(dt.data(), Size, Buffer, Size);
		SendQueue.push_back(std::move(dt));
	}
}