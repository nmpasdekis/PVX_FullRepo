#include <PVX_ComPort.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main() {
	PVX::Serial::Com Comm9(9);
	Comm9.Connect();

	uint8_t OnOff[]{
		1, 0, 0, 25,
		0, 0, 0, 0
	};

	while (1) {
		getchar();
		Comm9.Write(OnOff, 4);
		getchar();
		Comm9.Write(OnOff + 4, 4);
	}

	uint8_t Buffer[1025];
	Buffer[1024] = 0;

	while (1) {
		while (auto sz = Comm9.Read2(Buffer, 1024)) {
			Buffer[sz] = 0;
			std::cout << Buffer;
		}
		std::this_thread::sleep_for(10ms);
	}


	return 0;
}