#ifdef _WINDOWS

#include<Windows.h>
#include<PVX_File.h>

namespace fs = std::filesystem;

namespace PVX::IO{
	PathChangeEventer::PathChangeEventer(std::filesystem::path dir, std::function<void(const std::filesystem::path &)> clb, bool Start) : 
		Path{ dir.is_absolute()? dir : std::filesystem::current_path() / dir }, 
		Callback{ clb } {
		hDir = FindFirstChangeNotificationW(Path.wstring().c_str(), true, FILE_NOTIFY_CHANGE_LAST_WRITE);
		//DWORD read;
		//ReadDirectoryChangesW(hDir, Buffer.data(), 4096, 1, FILE_NOTIFY_CHANGE_LAST_WRITE, &read, 0, 0);
		if(Start) this->Start();
	}
	PathChangeEventer::~PathChangeEventer() {
		Running = false;
		if(th.has_value())
			th.value().join();
		CloseHandle(hDir);
	}
	
	void PathChangeEventer::Do() {
		std::lock_guard<std::mutex> lock{ mtx };
		DWORD res;
		res = WaitForSingleObject(hDir, 100);
		if(!res) {
			std::vector<fs::path> toDelete;
			for(const auto &[name, time] : Files) {
				if(fs::exists(name)) {
					auto ch = fs::last_write_time(name);
					if(ch != time) {
						Callback(Path / name);
						Files[name] = ch;
					}
				} else {
					toDelete.push_back(name);
				}
			}
			for(const auto &f : toDelete) Files.erase(f);
		}
		FindNextChangeNotification(hDir);
	}
	void PathChangeEventer::Start() {
		Running = true;
		th = std::thread([this] {
			while(Running) Do();
		});
	}

	void PathChangeEventer::Track(const std::filesystem::path &Filename) {
		if(fs::exists(Filename)) Files[Filename] = fs::last_write_time(Filename);
	}


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