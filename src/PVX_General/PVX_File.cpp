#define _CRT_SECURE_NO_WARNINGS

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

#ifdef __linux
#include <stdlib.h>
inline std::wstring toWide(const std::string& s) {
	std::wstring ret;
	ret.resize(s.size()*2);
	ret.resize(mbstowcs(ret.data(), s.data(), s.size()));
	return ret;
}
inline std::string fromWide(const std::wstring& s) {
	std::string ret;
	ret.resize(s.size()*4);
	ret.resize(wcstombs(ret.data(), s.data(), s.size()));
	return ret;
}
#endif


namespace PVX_Helpers {
	int indexOf(const char * s, char c, int start = 0);
}

namespace PVX {
	namespace IO {
		namespace fs = std::filesystem;

		void ChangeTracker::GetLastTime() {
			LastTime = fs::directory_entry(Filename).last_write_time();
		}
		ChangeTracker::ChangeTracker(const std::wstring& Filename) :
			Filename{ Filename },
			LastTime{ fs::directory_entry(Filename).last_write_time() } {}
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
						for (auto& t : Files)
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
		void ChangeEventer::Track(const std::wstring& Filename, std::function<void()> clb) {
			std::unique_lock<std::mutex> lock{ Locker };
			Files.push_back({ Filename, clb });
			if ((bool)Files.back().File)	clb();
		}

		void MakeDirectory(const std::wstring& Directory) {
			fs::create_directories(Directory);
		}
		void MakeDirectory(const std::string& Directory) {
			fs::create_directories(Directory);
		}

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
			return fs::directory_entry(filename).file_size();
		}
		size_t FileSize(const std::wstring & filename) {
			return fs::directory_entry(filename).file_size();
		}
		std::string FindFileFullPath(const std::string& Filename) {
			namespace fs = std::filesystem;
			auto p = fs::path{ Filename };
			if (p.is_absolute()) return Filename;
			auto cp = fs::current_path().string();
			cp.push_back(fs::path::preferred_separator);
			cp += Filename;
			if (FileExists(cp)) return cp;
			std::string_view var = getenv("PATH");
			while (var.size()) {
				auto x = var.find(';'); if (x==std::string_view::npos) x = var.size();
				auto pp = std::string(var.substr(0, x));
				if (pp.back() != fs::path::preferred_separator) pp.push_back(fs::path::preferred_separator);
				pp += Filename;
				if (FileExists(pp))
					return pp;
				var.remove_prefix(x+1);
			}
			return "";
		}


		std::wstring FindFileFullPath(const std::wstring& Filename) {
			namespace fs = std::filesystem;
			auto p = fs::path{ Filename };
			if (p.is_absolute()) return Filename;
			auto cp = fs::current_path().wstring();
			cp.push_back(fs::path::preferred_separator);
			cp += Filename;
			if (FileExists(cp)) return cp;
#ifdef __linux
			std::wstring PATH = [] { auto tmp = getenv("PATH");	return PVX::Decode::UTF((uint8_t*)tmp, std::strlen(tmp)); }();
			std::wstring_view var = PATH;
#else
			std::wstring_view var = _wgetenv(L"PATH");
#endif
			while (var.size()) {
				auto x = var.find(';'); if (x==std::wstring_view::npos) x = var.size();
				auto pp = std::wstring(var.substr(0, x));
				if (pp.back() != fs::path::preferred_separator) pp.push_back(fs::path::preferred_separator);
				pp += Filename;
				if (FileExists(pp))
					return pp;
				var.remove_prefix(x+1);
			}
			return L"";
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
			if (_wfopen_s(&file, Filename, L"rb")) return 0;
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
#ifndef __linux
			std::ifstream inp(Filename);
#else
			std::ifstream inp((const char*)PVX::Encode::UTF0(Filename).data());
#endif
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




		JSON::Item LoadJson(const char * Filename) {
			return JSON::parse(PVX::IO::ReadBinary(Filename));
		}
		JSON::Item LoadJson(const wchar_t * Filename) {
			return JSON::parse(PVX::IO::ReadBinary(Filename));
		}
		int Write(const std::wstring& Filename, const PVX::JSON::Item& Json) {
			return Write(Filename, PVX::Encode::UTF(PVX::JSON::stringify(Json)));
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
			int64_t i;
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

		std::vector<std::string> Dir(const std::string& filter) {
			std::vector<std::string> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (!entry.is_directory()) {
				auto fn = entry.path().string();
				size_t start = fn.size();
				for (; start>0 && fn[start-1] != '\\' && fn[start-1] != '/'; start--);
				ret.push_back(fn.substr(start));
			}
			return ret;
		}
		std::vector<std::string> DirFull(const std::string& filter) {
			std::vector<std::string> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (!entry.is_directory())
				ret.push_back(entry.path().string());
			return ret;
		}
		std::vector<std::string> SubDir(const std::string& filter) {
			std::vector<std::string> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (entry.is_directory()) {
				auto fn = entry.path().string();
				size_t start = fn.size();
				for (; start>0 && fn[start-1] != '\\' && fn[start-1] != '/'; start--);
				ret.push_back(fn.substr(start));
			}
			return ret;
		}
		std::vector<std::string> SubDirFull(const std::string& filter) {
			std::vector<std::string> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (entry.is_directory())
				ret.push_back(entry.path().string());
			return ret;
		}

		std::vector<std::wstring> Dir(const std::wstring& filter) {
			std::vector<std::wstring> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (!entry.is_directory()) {
				auto fn = entry.path().wstring();
				size_t start = fn.size();
				for (; start>0 && fn[start-1] != '\\' && fn[start-1] != '/'; start--);
				ret.push_back(fn.substr(start));
			}
			return ret;
		}
		std::vector<std::wstring> DirFull(const std::wstring& filter) {
			std::vector<std::wstring> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (!entry.is_directory()) {
				ret.push_back(entry.path().wstring());
			}
			return ret;
		}
		std::vector<std::wstring> SubDir(const std::wstring& filter) {
			std::vector<std::wstring> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (entry.is_directory()) {
				auto fn = entry.path().wstring();
				size_t start = fn.size();
				for (; start>0 && fn[start-1] != '\\' && fn[start-1] != '/'; start--);
				ret.push_back(fn.substr(start));
			}
			return ret;
		}
		std::vector<std::wstring> SubDirFull(const std::wstring& filter) {
			std::vector<std::wstring> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (entry.is_directory())
				ret.push_back(entry.path().wstring());
			return ret;
		}


		int FileExists(const std::wstring& file) {
			return fs::exists(file.c_str());
		}
		int FileExists(const std::string& file) {
			return fs::exists(file.c_str());
		}

		std::wstring wCurrentPath() {
			return fs::current_path().wstring();
		}
		std::string CurrentPath() {
			return fs::current_path().string();
		}
		void CurrentPath(const std::string path) {
			fs::current_path(path);
		}
		void CurrentPath(const std::wstring path) {
			fs::current_path(path);
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