#ifdef _WINDOWS

#include<Windows.h>
#include<PVX_File.h>


namespace PVX::IO{

	std::string OpenFileDialog(HWND Parent, const char* Filter, const char* Filename) {
		std::string fltr = Filter;
		fltr += '\0';
		for (auto& f : fltr) if (f == '|')f = 0;
		OPENFILENAMEA ofn{ 0 };
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = Parent;
		ofn.lpstrFile = new char[MAX_PATH];
		if (Filename)
			strcpy_s(ofn.lpstrFile, MAX_PATH - 1, Filename);
		else
			ofn.lpstrFile[0] = 0;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = fltr.c_str();
		ofn.nFilterIndex = 0;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		std::string ret;
		if (GetOpenFileNameA(&ofn)) {
			ret = ofn.lpstrFile;
		}
		delete ofn.lpstrFile;
		return ret;
	}
	std::wstring OpenFileDialog(HWND Parent, const wchar_t* Filter, const wchar_t* Filename) {
		std::wstring fltr = Filter;
		fltr += L'\0';
		for (auto& f : fltr) if (f == '|')f = 0;
		OPENFILENAMEW ofn{ 0 };
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = Parent;
		ofn.lpstrFile = new wchar_t[MAX_PATH];
		if (Filename)
			lstrcpyW(ofn.lpstrFile, Filename);
		else
			ofn.lpstrFile[0] = 0;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = fltr.c_str();
		ofn.nFilterIndex = 0;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		std::wstring ret;
		if (GetOpenFileNameW(&ofn)) {
			ret = ofn.lpstrFile;
		}
		delete ofn.lpstrFile;
		return ret;
	}
	std::string SaveFileDialog(HWND Parent, const char* Filter, const char* Filename) {
		std::string fltr = Filter;
		fltr += '\0';
		for (auto& f : fltr) if (f == '|')f = 0;
		OPENFILENAMEA ofn{ 0 };
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = Parent;
		ofn.lpstrFile = new char[MAX_PATH];
		if (Filename)
			strcpy_s(ofn.lpstrFile, MAX_PATH - 1, Filename);
		else
			ofn.lpstrFile[0] = 0;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = fltr.c_str();
		ofn.nFilterIndex = 0;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		std::string ret;
		if (GetSaveFileNameA(&ofn)) {
			ret = ofn.lpstrFile;
		}
		delete ofn.lpstrFile;
		return ret;
	}
	std::wstring SaveFileDialog(HWND Parent, const wchar_t* Filter, const wchar_t* Filename) {
		std::wstring fltr = Filter;
		fltr += L'\0';
		for (auto& f : fltr) if (f == '|')f = 0;
		OPENFILENAMEW ofn{ 0 };
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = Parent;
		ofn.lpstrFile = new wchar_t[MAX_PATH];
		if (Filename)
			lstrcpyW(ofn.lpstrFile, Filename);
		else
			ofn.lpstrFile[0] = 0;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = fltr.c_str();
		ofn.nFilterIndex = 0;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		std::wstring ret;
		if (GetSaveFileNameW(&ofn)) {
			ret = ofn.lpstrFile;
		}
		delete ofn.lpstrFile;
		return ret;
	}
}

#endif