#define NOMINMAX

#include <PVX_ComPort.h>
#include <algorithm>
#include <setupapi.h>
#include <devguid.h>


namespace PVX::Serial {
	std::string commNaming(uint8_t n) {
		using namespace std::string_literals;
		if (n <= 9) return "COM"s + std::to_string(n);
		return "\\\\.\\COM"s + std::to_string(n);
	}

	Com::Com(uint8_t Num, uint32_t BaudRate) {
		comNumber = Num;
		commName = commNaming(Num);
		baudRate = BaudRate;
	}

	uint32_t Com::Connect() {
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
				}
			}
		}
		return Error;
	}

	void Com::Disconnect() {
		if (hCom) CloseHandle(hCom);
		hCom = NULL;
	}

	std::unordered_map<int, std::string> Com::List() {
		using namespace std::string_literals;
		std::unordered_map<int, std::string> ret;
		char Text[1024];
		for (int i = 0; i < 255; i++) {
			std::string Name = "COM"s + std::to_string(i);
			DWORD confSize = 1024;
			int sz = QueryDosDeviceA(Name.data(), Text, 1024);
			if (sz) {
				ret[i] = std::string(Text);
			}
		}
		return ret;
	}

	size_t Com::WaitForInput() {
		auto ret = AvailableInBytes();
		if (ret) return ret;
		DWORD mask = 0;
		while (!(mask & EV_RXCHAR) && !ret) {
			mask = 0;
			auto tst = WaitCommEvent(hCom, &mask, NULL);
			if(!tst) {
				tst = GetLastError();
				tst = 123;
			}
			ret = AvailableInBytes();
		}
		return ret;
	}

	size_t Com::WaitForOutput() {
		DWORD mask = 0;
		while (!(mask & EV_TXEMPTY)) {
			mask = 0;
			auto tst = WaitCommEvent(hCom, &mask, NULL);
			if(!tst) {
				tst = GetLastError();
				tst = 123;
			}
		}
		return 0;
	}

	size_t Com::Read(uint8_t* Buffer, uint32_t Size) {
		DWORD ret = 0;
		if(!this) 
			return 0;

		if (ReadFile(hCom, Buffer, Size, &ret, NULL)) {
			return ret;
		} else {
			Error = GetLastError();
			return 0;
		}
		return 0;
	}
	size_t Com::Read(void* Buffer, uint32_t Size) {
		DWORD ret = 0;
		if(!this) 
			return 0;

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
		if(!this) 
			return 0;
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
		if(!this) 
			return 0;
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
		if(!this) 
			return 0;
		ClearCommError(hCom, &Error, &status);
		if (Error) {
			Error = GetLastError();
			return 0;
		}
		return status.cbInQue;
	}
	///*
	std::vector<PortInfo> Com::Ports()
	{
		std::vector<PortInfo> devices_found;

		HDEVINFO device_info_set = SetupDiGetClassDevs((const GUID *) &GUID_DEVCLASS_PORTS,0,0,DIGCF_PRESENT);

		unsigned int index = 0;
		SP_DEVINFO_DATA device_info_data;

		device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

		while(SetupDiEnumDeviceInfo(device_info_set, index++, &device_info_data)) {
			HKEY hkey = SetupDiOpenDevRegKey(
				device_info_set,
				&device_info_data,
				DICS_FLAG_GLOBAL,
				0,
				DIREG_DEV,
				KEY_READ);

			char name[256];
			DWORD sz = 256;

			LONG return_code = RegQueryValueExA(hkey, "PortName", 0, 0, (uint8_t*)name, &sz);

			RegCloseKey(hkey);

			if(return_code != EXIT_SUCCESS) continue;

			if(sz > 0 && sz <= 256) name[sz-1] = '\0'; 
			else name[0] = '\0';

			if(strstr(name, "LPT") != NULL) continue;

			// Get port friendly name

			char fname[256];
			DWORD fsz = 0;

			BOOL rez = SetupDiGetDeviceRegistryPropertyA(device_info_set, &device_info_data, SPDRP_FRIENDLYNAME, 0, (uint8_t*)fname, 256, &fsz);

			if(rez == TRUE && fsz > 0) fname[fsz-1] = '\0';
			else fname[0] = '\0';

			// Get hardware ID

			char id[256];
			DWORD idsz = 0;

			BOOL got_hardware_id = SetupDiGetDeviceRegistryPropertyA(device_info_set, &device_info_data, SPDRP_HARDWAREID, 0, (uint8_t*)id, 256, &idsz);

			if(got_hardware_id == TRUE && idsz > 0) id[idsz-1] = '\0';
			else id[0] = '\0';

			devices_found.push_back({ name+3, fname, id });
		}

		SetupDiDestroyDeviceInfoList(device_info_set);

		return devices_found;
	}
	//*/
}