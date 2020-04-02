#include<Windows.h>
#include<vector>
#include<string>
#include<PVX_File.h>
#include<stdio.h>
#pragma comment(lib, "User32.lib")
#include<fstream>
#include<PVX_Encode.h>
#include<PVX_String.h>
#include<PVX_Regex.h>
#include <PVX.inl>
#include<mutex>

namespace PVX_Helpers {
	int indexOf(const char * s, char c, int start = 0);
}

namespace PVX {
	namespace IO {
		std::wstring ReadUtf(const std::wstring& Filename) {
			auto bin = ReadBinary(Filename.c_str());
			
			return PVX::Decode::UTF(bin);
		}
		size_t FileSize(FILE * fin) {
			size_t cur = ftell(fin);
			fseek(fin, 0, SEEK_END);
			size_t ret = ftell(fin);
			fseek(fin, cur, SEEK_SET);
			return ret;
		}
		size_t FileSize(const std::string & filename) {
			FILE * fin;
			if (fopen_s(&fin, filename.c_str(), "rb")) return 0;
			auto ret = FileSize(fin);
			fclose(fin);
			return ret;
		}
		size_t FileSize(const std::wstring & filename) {
			FILE * fin;
			if (_wfopen_s(&fin, filename.c_str(), L"rb")) return 0;
			auto ret = FileSize(fin);
			fclose(fin);
			return ret;
		}
		std::string FindFileFullPath(const std::string& Filename) {
			if (Filename.size()) {
				if (Filename.find(":\\")!=std::string::npos) return Filename;
				auto fn = [&] {
					if (Filename[0]=='\\')
						return Filename.substr(1);
					return Filename;
				}();
				if (FileExists(fn)) return CurrentPath()+ "\\" + fn;
				size_t buffSize;
				getenv_s(&buffSize, nullptr, 0, "PATH");
				if (buffSize > 1) {
					std::string var;
					var.resize(buffSize-1);
					getenv_s(&buffSize, &var[0], buffSize, "PATH");
					size_t dbg = 0;
					auto paths = PVX::Map(PVX::String::Split_No_Empties_Trimed(var, ";"), [&](const std::string& s) {
						dbg++;
						if (s.back()!='\\')
							return s+"\\";
						return s;
					});
					for (auto& p : paths) {
						std::string ret = p+fn;
						if (FileExists(ret))
							return ret;
					}
				}
			}
			return std::string();
		}
		std::wstring FindFileFullPath(const std::wstring& Filename) {
			if (Filename.size()) {
				if (Filename.find(L":\\")!=std::string::npos) return Filename;
				auto fn = [&] {
					if (Filename[0]==L'\\')
						return Filename.substr(1);
					return Filename;
				}();
				if (FileExists(fn)) return wCurrentPath() + L"\\" + fn;
				size_t buffSize;
				_wgetenv_s(&buffSize, nullptr, 0, L"PATH");
				if (buffSize > 1) {
					std::wstring var;
					var.resize(buffSize-1);
					_wgetenv_s(&buffSize, &var[0], buffSize, L"PATH");
					size_t dbg = 0;
					auto paths = PVX::Map(PVX::String::Split_No_Empties_Trimed(var, L";"), [&](const std::wstring& s) {
						dbg++;
						if (s.back()!=L'\\')
							return s + L"\\";
						return s;
					});
					for (auto& p : paths) {
						std::wstring ret = p+fn;
						if (FileExists(ret))
							return ret;
					}
				}
			}
			return std::wstring();
		}
		std::string FilePathPart(const std::string& Filename) {
			auto p = PVX::String::Split(FindFileFullPath(Filename), "\\");
			p.pop_back();
			return PVX::String::Join(p, "\\");
		}
		std::wstring FilePathPart(const std::wstring& Filename) {
			auto p = PVX::String::Split(FindFileFullPath(Filename), L"\\");
			p.pop_back();
			return PVX::String::Join(p, L"\\");
		}

		int Write(const std::string & fn, const void*data, size_t Size) {
			FILE * fout;
			if(fopen_s(&fout, fn.c_str(), "wb"))return 0;
			fwrite(data, 1, Size, fout);
			fclose(fout);
			return 1;
		}
		int Write(const std::string & fn, const std::vector<unsigned char> & Data) {
			return Write(fn, Data.data(), Data.size());
		}

		int Write(const std::wstring & fn, const void*data, size_t Size) {
			FILE * fout;
			if(_wfopen_s(&fout, fn.c_str(), L"wb"))return 0;
			fwrite(data, 1, Size, fout);
			fclose(fout);
			return 1;
		}
		int Write(const std::wstring & fn, const std::vector<unsigned char> & Data) {
			return Write(fn, Data.data(), Data.size());
		}

		std::vector<unsigned char> ReadBinary(const char * Filename) {
			FILE * file;
			std::vector<unsigned char> ret;
			if(fopen_s(&file, Filename, "rb"))return ret;
			fseek(file, 0, SEEK_END);
			ret.resize(ftell(file));
			if (ret.size()) {
				fseek(file, 0, SEEK_SET);
				fread(&ret[0], 1, ret.size(), file);
			}
			fclose(file);
			return ret;
		}
		std::vector<unsigned char> ReadBinary(const char * Filename, size_t offset, size_t length) {
			FILE * file;
			std::vector<unsigned char> ret(length);
			if(fopen_s(&file, Filename, "rb"))return ret;
			fseek(file, offset, SEEK_SET);
			fread(&ret[0], 1, length, file);
			fclose(file);
			return ret;
		}
		size_t ReadBinary(const char * Filename, std::vector<unsigned char> & Data) {
			FILE * file;
			if(fopen_s(&file, Filename, "rb")) return 0;
			Data.clear();
			fseek(file, 0, SEEK_END);
			size_t sz = ftell(file);
			fseek(file, 0, SEEK_SET);
			Data.resize(sz);
			int ret = (sz == fread(&Data[0], 1, sz, file));
			fclose(file);
			if(ret)
				return sz;
			return 0;
		}
		size_t ReadBinary(const char * Filename, size_t offset, size_t length, std::vector<unsigned char> & Data) {
			FILE * file;
			if(fopen_s(&file, Filename, "rb")) return 0;
			Data.clear();
			Data.resize(length);
			fseek(file, offset, SEEK_SET);
			int ret = (length == fread(&Data[0], 1, length, file));
			fclose(file);
			if(ret)return length;
			return 0;
		}

		std::vector<unsigned char> ReadBinary(const wchar_t * Filename) {
			FILE * file;
			std::vector<unsigned char> ret;
			if(_wfopen_s(&file, Filename, L"rb"))return ret;
			fseek(file, 0, SEEK_END);
			ret.resize(ftell(file));
			if (ret.size()) {
				fseek(file, 0, SEEK_SET);
				fread(&ret[0], 1, ret.size(), file);
			}
			fclose(file);
			return ret;
		}
		std::vector<unsigned char> ReadBinary(const wchar_t * Filename, size_t offset, size_t length) {
			FILE * file;
			std::vector<unsigned char> ret(length);
			if(_wfopen_s(&file, Filename, L"rb"))return ret;
			fseek(file, offset, SEEK_SET);
			fread(&ret[0], 1, length, file);
			fclose(file);
			return ret;
		}
		size_t ReadBinary(const wchar_t * Filename, std::vector<unsigned char> & Data) {
			FILE * file;
			if(_wfopen_s(&file, Filename, L"rb")) return 0;
			Data.clear();
			fseek(file, 0, SEEK_END);
			size_t sz = ftell(file);
			fseek(file, 0, SEEK_SET);
			Data.resize(sz);
			int ret = (sz == fread(&Data[0], 1, sz, file));
			fclose(file);
			if(ret)
				return sz;
			return 0;
		}
		size_t ReadBinary(const wchar_t * Filename, size_t offset, size_t length, std::vector<unsigned char> & Data) {
			FILE * file;
			if(_wfopen_s(&file, Filename, L"rb")) return 0;
			Data.clear();
			Data.resize(length);
			fseek(file, offset, SEEK_SET);
			int ret = (length == fread(&Data[0], 1, length, file));
			fclose(file);
			if(ret)return length;
			return 0;
		}

		std::string ReadText(const char * Filename) {
			std::ifstream inp(Filename);
			if(inp.fail())return "";
			std::string txt(std::istreambuf_iterator<char>(inp), (std::istreambuf_iterator<char>()));
			inp.close();
			return txt;
		}
		std::string ReadText(const wchar_t* Filename) {
			std::ifstream inp(Filename);
			if (inp.fail())return "";
			std::string txt(std::istreambuf_iterator<char>(inp), (std::istreambuf_iterator<char>()));
			inp.close();
			return txt;
		}

		std::vector<std::string> SplitPath(const std::string & Path) {
			return PVX::String::Split_No_Empties(Path, "\\");
		}

		std::vector<std::wstring> SplitPath(const std::wstring & Path) {
			return PVX::String::Split_No_Empties(Path, L"\\");
		}

		std::string ReplaceExtension(const std::string & Filename, const std::string & NewExtension) {
			auto spl = PVX::String::Split(Filename, ".");
			spl.pop_back();
			if (NewExtension.size()) spl.push_back(NewExtension);
			return PVX::String::Join(spl, ".");
		}

		std::wstring ReplaceExtension(const std::wstring & Filename, const std::wstring & NewExtension) {
			auto spl = PVX::String::Split(Filename, L".");
			spl.pop_back();
			if(NewExtension.size()) spl.push_back(NewExtension);
			return PVX::String::Join(spl, L".");
		}

		std::vector<std::string> Dir(const std::string & Expression) {
			std::vector<std::string> ret;
			WIN32_FIND_DATAA ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			hFind = FindFirstFileA(Expression.data(), &ffd);
			if(hFind != INVALID_HANDLE_VALUE) {
				do {
					if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						ret.push_back(ffd.cFileName);
					}
				} while(FindNextFileA(hFind, &ffd) != 0);
			}
			return ret;
		}
		std::vector<std::wstring> Dir(const std::wstring & Expression) {
			std::vector<std::wstring> ret;
			WIN32_FIND_DATAW ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			hFind = FindFirstFileW(Expression.data(), &ffd);
			if(hFind != INVALID_HANDLE_VALUE) {
				do {
					if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						ret.push_back(ffd.cFileName);
					}
				} while(FindNextFileW(hFind, &ffd) != 0);
			}
			return ret;
		}

		std::vector<std::string> SubDir(const std::string & Expression) {
			std::vector<std::string> ret;
			WIN32_FIND_DATAA ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			hFind = FindFirstFileA(Expression.data(), &ffd);
			if(hFind != INVALID_HANDLE_VALUE) {
				do {
					if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && (ffd.cFileName[0] != '.' || ffd.cFileName[1])) {
						ret.push_back(ffd.cFileName);
					}
				} while(FindNextFileA(hFind, &ffd) != 0);
			}
			return ret;
		}
		std::vector<std::wstring> SubDir(const std::wstring & Expression) {
			std::vector<std::wstring> ret;
			WIN32_FIND_DATAW ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			hFind = FindFirstFileW(Expression.data(), &ffd);
			if(hFind != INVALID_HANDLE_VALUE) {
				do {
					if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && (ffd.cFileName[0] != L'.' || ffd.cFileName[1])) {
						ret.push_back(ffd.cFileName);
					}
				} while(FindNextFileW(hFind, &ffd) != 0);
			}
			return ret;
		}

		std::vector<std::string> DirFull(const std::string & Expression) {
			std::vector<std::string> ret;
			WIN32_FIND_DATAA ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			int index = -1;
			for(int i = 0; i < Expression.size(); i++) {
				if(Expression[i] == '\\' || Expression[i] == '/')
					index = i;
			}

			std::string d = "";
			if(index != -1)
				d = Expression.substr(0, index + 1);

			hFind = FindFirstFileA(Expression.data(), &ffd);
			if(hFind != INVALID_HANDLE_VALUE) {
				do {
					if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						std::string fl = d;
						fl += ffd.cFileName;
						ret.push_back(fl);
					}
				} while(FindNextFileA(hFind, &ffd) != 0);
			}
			return ret;
		}
		std::vector<std::wstring> DirFull(const std::wstring & Expression) {
			std::vector<std::wstring> ret;
			WIN32_FIND_DATAW ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			int index = -1;
			for(int i = 0; i < Expression.size(); i++) {
				if(Expression[i] == L'\\' || Expression[i] == L'/')
					index = i;
			}

			std::wstring d = L"";
			if(index != -1)
				d = Expression.substr(0, index + 1);

			hFind = FindFirstFileW(Expression.data(), &ffd);
			if(hFind != INVALID_HANDLE_VALUE) {
				do {
					if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						std::wstring fl = d;
						fl += ffd.cFileName;
						ret.push_back(fl);
					}
				} while(FindNextFileW(hFind, &ffd) != 0);
			}
			return ret;
		}

		std::vector<std::string> SubDirFull(const std::string & Expression) {
			std::vector<std::string> ret;
			WIN32_FIND_DATAA ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			int index = -1;
			for(int i = 0; i < Expression.size(); i++) {
				if(Expression[i] == '\\' || Expression[i] == '/')
					index = i;
			}

			std::string d = "";
			if(index != -1)
				d = Expression.substr(0, index + 1);

			hFind = FindFirstFileA(Expression.data(), &ffd);
			if(hFind != INVALID_HANDLE_VALUE) {
				do {
					if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && (ffd.cFileName[0] != '.' || ffd.cFileName[1])) {
						std::string fl = d;
						fl += ffd.cFileName;
						ret.push_back(fl);
					}
				} while(FindNextFileA(hFind, &ffd) != 0);
			}
			return ret;
		}
		std::vector<std::wstring> SubDirFull(const std::wstring & Expression) {
			std::vector<std::wstring> ret;
			WIN32_FIND_DATAW ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			int index = -1;
			for(int i = 0; i < Expression.size(); i++) {
				if(Expression[i] == L'\\' || Expression[i] == L'/')
					index = i;
			}

			std::wstring d = L"";
			if(index != -1)
				d = Expression.substr(0, index + 1);

			hFind = FindFirstFileW(Expression.data(), &ffd);
			if(hFind != INVALID_HANDLE_VALUE) {
				do {
					if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && (ffd.cFileName[0] != L'.' || ffd.cFileName[1])) {
						std::wstring fl = d;
						fl += ffd.cFileName;
						ret.push_back(fl);
					}
				} while(FindNextFileW(hFind, &ffd) != 0);
			}
			return ret;
		}

		int FileExists(const std::wstring & file) {
			WIN32_FIND_DATAW FindFileData;
			HANDLE handle = FindFirstFileW(file.c_str(), &FindFileData);
			if (handle != INVALID_HANDLE_VALUE) {
				FindClose(handle);
				return 1;
			}
			return 0;
		}
		int FileExists(const std::string & file) {
			WIN32_FIND_DATAA FindFileData;
			HANDLE handle = FindFirstFileA(file.c_str(), &FindFileData);
			if (handle != INVALID_HANDLE_VALUE) {
				FindClose(handle);
				return 1;
			}
			return 0;
		}

		void MakeDirectory(const std::wstring & Directory) {
			auto path = PVX::String::Split_No_Empties(PVX::Replace(Directory, L"/", L"\\"), L"\\");
			size_t i;
			std::wstring cur;
			for (i = 0; i < path.size(); i++) {
				cur += path[i];
				if (!FileExists(cur))
					CreateDirectoryW(cur.c_str(), NULL);
				cur += L"\\";
			}
		}
		void MakeDirectory(const std::string & Directory) {
			auto path = PVX::String::Split_No_Empties(PVX::Replace(Directory, "/", "\\"), "\\");
			size_t i;
			std::string cur;
			for (i = 0; i < path.size(); i++) {
				cur += path[i];
				if (!FileExists(cur))
					CreateDirectoryA(cur.c_str(), NULL);
				cur += "\\";
			}
		}

		std::string OpenFileDialog(HWND Parent, const char * Filter, const char * Filename) {
			std::string fltr = Filter;
			fltr += '\0';
			for(auto & f : fltr) if(f == '|')f = 0;
			OPENFILENAMEA ofn{ 0 };
			ofn.lStructSize = sizeof(OPENFILENAMEA);
			ofn.hwndOwner = Parent;
			ofn.lpstrFile = new char[MAX_PATH];
			if(Filename)
				strcpy_s(ofn.lpstrFile, MAX_PATH - 1, Filename);
			else
				ofn.lpstrFile[0] = 0;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFilter = fltr.c_str();
			ofn.nFilterIndex = 0;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			std::string ret;
			if(GetOpenFileNameA(&ofn)) {
				ret = ofn.lpstrFile;
			}
			delete ofn.lpstrFile;
			return ret;
		}
		std::wstring OpenFileDialog(HWND Parent, const wchar_t * Filter, const wchar_t * Filename) {
			std::wstring fltr = Filter;
			fltr += L'\0';
			for (auto & f : fltr) if (f == '|')f = 0;
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
		std::string SaveFileDialog(HWND Parent, const char * Filter, const char * Filename) {
			std::string fltr = Filter;
			fltr += '\0';
			for(auto & f : fltr) if(f == '|')f = 0;
			OPENFILENAMEA ofn{ 0 };
			ofn.lStructSize = sizeof(OPENFILENAMEA);
			ofn.hwndOwner = Parent;
			ofn.lpstrFile = new char[MAX_PATH];
			if(Filename)
				strcpy_s(ofn.lpstrFile, MAX_PATH - 1, Filename);
			else
				ofn.lpstrFile[0] = 0;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFilter = fltr.c_str();
			ofn.nFilterIndex = 0;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			std::string ret;
			if(GetSaveFileNameA(&ofn)) {
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
		JSON::Item LoadJson(const char * Filename) {
			return JSON::parse(PVX::IO::ReadBinary(Filename));
		}
		JSON::Item LoadJson(const wchar_t * Filename) {
			return JSON::parse(PVX::IO::ReadBinary(Filename));
		}
		std::wstring wCurrentPath() {
			wchar_t Path[MAX_PATH + 1];
			GetCurrentDirectoryW(MAX_PATH, Path);
			return Path;
		}
		std::string CurrentPath() {
			char Path[MAX_PATH + 1];
			GetCurrentDirectoryA(MAX_PATH, Path);
			return Path;
		}
		void CurrentPath(const std::string path) {
			SetCurrentDirectoryA(path.c_str());
		}
		void CurrentPath(const std::wstring path) {
			SetCurrentDirectoryW(path.c_str());
		}

		std::vector<std::string> FileExtensions(const std::string & f) {
			size_t dot = f.size();
			for(auto i = 0; i < f.size(); i++) if(f[i] == '.')dot = i;
			std::vector<std::string> ret;
			ret.push_back(f.substr(0, dot));
			if(dot == f.size())
				ret.push_back("");
			else
				ret.push_back(f.substr(dot + 1, f.size() - dot - 1));
			return ret;
		}
		std::vector<std::wstring> FileExtensions(const std::wstring & f) {
			size_t dot = f.size();
			for(auto i = 0; i < f.size(); i++) if(f[i] == L'.')dot = i;
			std::vector<std::wstring> ret;
			ret.push_back(f.substr(0, dot));
			if(dot == f.size())
				ret.push_back(L"");
			else
				ret.push_back(f.substr(dot + 1, f.size() - dot - 1));
			return ret;
		}

		std::string FileExtension(const std::string & f) {
			size_t dot = f.size();
			for (auto i = 0; i < f.size(); i++) if (f[i] == '.')dot = i;
			return (dot == f.size()) ? "" : f.substr(dot + 1, f.size() - dot - 1);
		}
		std::wstring FileExtension(const std::wstring & f) {
			size_t dot = f.size();
			for (auto i = 0; i < f.size(); i++) if (f[i] == L'.')dot = i;
			return (dot == f.size())? L"": f.substr(dot + 1, f.size() - dot - 1);
		}


		Text::Text(const char * Filename) : BufferPosition(0), BufferSize(0){
			fopen_s(&fin, Filename, "rb");
		}
		Text::Text(const wchar_t * Filename) : BufferPosition(0), BufferSize(0) {
			_wfopen_s(&fin, Filename, L"rb");
		}
		size_t Text::ReadLine() {
			long long i;
			std::vector<unsigned char> Data;
			do {
				if(BufferSize == BufferPosition) {
					BufferSize = fread_s(buffer, 512, 1, 512, fin);
					if(!BufferSize)return 0;
					BufferPosition = 0;
				}
				for(i = BufferPosition; i < BufferSize && buffer[i] != '\n'; i++);
				size_t sz = i - BufferPosition;
				if(sz) {
					size_t oldSize = Data.size();
					Data.resize(oldSize + sz);
					memcpy(&Data[oldSize], buffer + BufferPosition, sz);
				}
				BufferPosition += sz + (buffer[i] == '\n');
			} while(BufferSize == 512 && buffer[i]!='\n');
			curLine = PVX::Decode::UTF(Data);
			return curLine.size();
		}
		std::wstring Text::Line() {
			return curLine;
		}


		BinReader::BinReader(const std::string & Filename) {
			pData = new PrivateData{ 1 };
			if(!fopen_s(&pData->fin, Filename.c_str(), "rb")) {
				fseek(pData->fin, 0, SEEK_END);
				pData->_Size = ftell(pData->fin);
				fseek(pData->fin, 0, SEEK_SET);
			}
		}
		BinReader::BinReader(const std::wstring & Filename) {
			pData = new PrivateData{ 1 };
			if (!_wfopen_s(&pData->fin, Filename.c_str(), L"rb")) {
				fseek(pData->fin, 0, SEEK_END);
				pData->_Size = ftell(pData->fin);
				fseek(pData->fin, 0, SEEK_SET);
			}
		}
		BinReader::BinReader(const BinReader & b) {
			pData = b.pData;
			pData->RefCount++;
		}
		BinReader & BinReader::operator=(const BinReader & b) {
			if (!(--pData->RefCount)) {
				fclose(pData->fin);
				delete pData;
			}
			pData = b.pData;
			pData->RefCount++;
			return *this;
		}
		BinReader::~BinReader() {
			if (!(--pData->RefCount)) {
				fclose(pData->fin);
				delete pData;
			}
		}
		size_t BinReader::Read(void * Data, int ByteCount) {
			return fread_s(Data, ByteCount, 1, ByteCount, pData->fin);
		}
		size_t BinReader::Read(void * Data, int ElementSize, int ElementCount) {
			return fread_s(Data, ElementSize * ElementCount, ElementSize, ElementCount, pData->fin);
		}
		void BinReader::Skip(size_t nBytes) {
			fseek(pData->fin, nBytes, SEEK_CUR);
		}
		int BinReader::Eof() const {
			return feof(pData->fin);
		}
		int BinReader::OK() const {
			return pData->fin != 0;
		}
		size_t BinReader::Size() const {
			return pData->_Size;
		}
		size_t BinReader::RemainingBytes() const {
			return pData->_Size - ftell(pData->fin);
		}
		size_t BinReader::CurrentPosition() const {
			return ftell(pData->fin);
		}
		void BinReader::CurrentPosition(size_t pos) {
			fseek(pData->fin, pos, SEEK_SET);
		}
		std::vector<unsigned char> BinReader::Read(size_t Offset, size_t Count) {
			std::vector<unsigned char> ret;
			if (Offset < pData->_Size) {
				if (Offset + Count > pData->_Size)
					Count = pData->_Size - Offset;
				ret.resize(Count);
				fseek(pData->fin, Offset, SEEK_SET);
				auto sz = fread_s(&ret[0], Count, 1, Count, pData->fin);
				ret.resize(sz);
			}
			return ret;
		}
		std::string BinReader::ReadString(size_t Offset, size_t Count) {
			std::string ret;
			if (Offset < pData->_Size) {
				if (Offset + Count > pData->_Size)
					Count = pData->_Size - Offset;
				ret.resize(Count);
				fseek(pData->fin, Offset, SEEK_SET);
				auto sz = fread_s(&ret[0], Count, 1, Count, pData->fin);
				ret.resize(sz);
			}
			return ret;
		}
		void ChangeTracker::GetLastTime() {
			hFile = CreateFileW(Filename.c_str(), GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
			GetFileTime(hFile, 0, (LPFILETIME)&LastTime, 0);
			CloseHandle(hFile);
		}
		ChangeTracker::ChangeTracker(const std::wstring & Filename) : Filename{ Filename }, LastTime{ 0 } {
			//GetLastTime();
		}
		ChangeTracker::operator bool() {
			auto lt = LastTime;
			GetLastTime();
			return lt != LastTime;
		}
		ChangeTracker::operator std::wstring() {
			return Filename;
		}
		ChangeEventer::ChangeEventer() {
			Running = 1;
			Tracker = std::thread([this]() {
				while (Running) {
					{
						std::unique_lock<std::mutex> lock{ Locker };
						for (auto & t : Files)
							if (t.File) t.Do();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(333));
				}
			});
		}
		ChangeEventer::~ChangeEventer() {
			Running = 0;
			Tracker.join();
		}
		void ChangeEventer::Track(const std::wstring & Filename, std::function<void()> clb) {
			std::unique_lock<std::mutex> lock{ Locker };
			Files.push_back({ Filename, clb });
			if((bool)Files.back().File)	clb();
		}
	}
	BinWriter::BinWriter(const std::string_view& Filename) :fout{0} {
		fopen_s(&fout, Filename.data(), "wb");
	}
	BinWriter::BinWriter(const std::wstring_view& Filename) {
		_wfopen_s(&fout, Filename.data(), L"wb");
	}
	BinWriter::~BinWriter() {
		fclose(fout);
	}
	int BinWriter::Write(void* data, size_t sz) {
		return fwrite(data, 1, sz, fout);
	}
}