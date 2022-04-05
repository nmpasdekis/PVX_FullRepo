#ifndef __PVX_FILE_H__
#define __PVX_FILE_H__

#ifndef __linux
#include<Windows.h>
#endif

#include<vector>
#include<string>
#include<PVX_json.h>
#include<thread>
#include<functional>
#include<mutex>
#include<fstream>
#include <filesystem>
#include<PVX_Encode.h>

namespace PVX {
	namespace IO {
		constexpr wchar_t sep = std::filesystem::path::preferred_separator;
		constexpr wchar_t otherSep = [] { return sep == L'\\' ? L'/' : L'\\'; }();
		constexpr wchar_t sepString[2]{ sep, 0 };
		constexpr wchar_t otherSepString[2]{ otherSep, 0 };

		class ChangeTracker {
			std::wstring Filename;
			std::filesystem::file_time_type LastTime;
			void GetLastTime();
		public:
			ChangeTracker(const std::wstring& Filename);
			operator bool();
			operator std::wstring();
		};

		class ChangeEventer {
			struct Events {
				ChangeTracker File;
				std::function<void()> Do;
			};
			std::mutex Locker;
			std::thread Tracker;
			std::vector<Events> Files;
			int Running;
		public:
			ChangeEventer();
			~ChangeEventer();
			void Track(const std::wstring& Filename, std::function<void()> clb);
		};
		class Text {
			unsigned char buffer[512];
			size_t BufferPosition, BufferSize;
			FILE* fin;
			std::wstring curLine;
		public:
			Text(const char* Filename);
			Text(const wchar_t* Filename);
			size_t ReadLine();
			std::wstring Line();
		};

		std::wstring ReadUtf(const std::wstring& Filename);

		class BinReader {
			struct PrivateData {
				int RefCount;
				FILE* fin;
				size_t _Size;
			};
			PrivateData* pData;
		public:
			BinReader(const std::string& Filename);
			BinReader(const std::wstring& Filename);
			BinReader(const BinReader& b);
			BinReader& operator=(const BinReader& b);
			~BinReader();
			size_t Read(void* Data, int ByteCount);
			size_t Read(void* Data, int ElementSize, int ElementCount);
			void Skip(size_t nBytes);
			int Eof() const;
			int OK() const;
			size_t Size() const;
			size_t RemainingBytes() const;

			size_t CurrentPosition() const;
			void CurrentPosition(size_t pos);

			std::vector<unsigned char> Read(size_t Offset, size_t Count);
			std::string ReadString(size_t Offset, size_t Count);

			template<typename T>
			T Read() {
				T ret;
				Read(&ret, sizeof(T));
				return ret;
			}

			template<typename T>
			int ReadArray(std::vector<T>& Out, int ElementCount) {
				Out.resize(ElementCount);
				return Read(&Out[0], sizeof(T), ElementCount) == ElementCount;
			}

			template<typename T>
			int Read(T& Out) {
				return Read(&Out, sizeof(T));;
			}

			template<typename T>
			std::vector<T> ReadArray(int ItemCount) {
				std::vector<T> ret(ItemCount);
				Read(&ret[0], ItemCount * sizeof(T));
				return ret;
			}

		};

		inline void ReadLines(const std::string& Filename, std::function<void(const std::string&)> clb) {
			std::ifstream fin(Filename.c_str());
			if (fin.fail())return;
			std::string line;
			while (!fin.eof()) {
				std::getline(fin, line);
				clb(line);
			}
		}

		inline void ReadLines(const std::wstring& Filename, std::function<void(const std::string&)> clb) {
#ifndef __linux
			std::ifstream fin(Filename.c_str());
#else
			std::ifstream fin((char*)PVX::Encode::UTF0(Filename.c_str()).data());
#endif
			if (fin.fail())return;
			std::string line;
			while (!fin.eof()) {
				std::getline(fin, line);
				clb(line);
			}
		}

		inline void ReadLinesUTF(const std::string& Filename, std::function<void(const std::wstring&)> clb) {
			std::ifstream fin(Filename.c_str());
			if (fin.fail())return;
			std::string line;
			while (!fin.eof()) {
				std::getline(fin, line);
				clb(PVX::Decode::UTF((unsigned char*)line.c_str(), line.size()));
			}
		}

		inline void ReadLinesUTF(const std::wstring& Filename, std::function<void(const std::wstring&)> clb) {
#ifndef __linux
			std::ifstream fin(Filename.c_str());
#else
			std::ifstream fin((char*)PVX::Encode::UTF0(Filename.c_str()).data());
#endif
			if (fin.fail())return;
			std::string line;
			while (!fin.eof()) {
				std::getline(fin, line);
				clb(PVX::Decode::UTF((unsigned char*)line.c_str(), line.size()));
			}
		}

		size_t FileSize(FILE* fin);
		size_t FileSize(const std::string& filename);
		size_t FileSize(const std::wstring& filename);

		std::string FindFileFullPath(const std::string& Filename);
		std::wstring FindFileFullPath(const std::wstring& Filename);

		std::string FilePathPart(const std::string& Filename);
		std::wstring FilePathPart(const std::wstring& Filename);

		int Write(const std::string& fn, const void* data, size_t Size);
		int Write(const std::string& fn, const std::vector<unsigned char>& Data);
		int Write(const std::string& fn, const PVX::JSON::Item& Data);

		int Write(const std::wstring& fn, const void* data, size_t Size);
		int Write(const std::wstring& fn, const std::vector<unsigned char>& Data);
		int Write(const std::wstring& fn, const PVX::JSON::Item& Data);

		std::vector<unsigned char> ReadBinary(const char* Filename);
		std::vector<unsigned char> ReadBinary(const char* Filename, size_t offset, size_t length);
		size_t ReadBinary(const char* Filename, std::vector<unsigned char>& Data);
		size_t ReadBinary(const char* Filename, size_t offset, size_t length, std::vector<unsigned char>& Data);
		std::vector<unsigned char> ReadBinary(const wchar_t* Filename);
		std::vector<unsigned char> ReadBinary(const wchar_t* Filename, size_t offset, size_t length);
		size_t ReadBinary(const wchar_t* Filename, std::vector<unsigned char>& Data);
		size_t ReadBinary(const wchar_t* Filename, size_t offset, size_t length, std::vector<unsigned char>& Data);
		std::string ReadText(const char* Filename);
		std::string ReadText(const wchar_t* Filename);
		std::vector<std::string> Dir(const std::string& Expression);
		std::vector<std::wstring> Dir(const std::wstring& Expression);
		std::vector<std::string> DirFull(const std::string& Expression);
		std::vector<std::wstring> DirFull(const std::wstring& Expression);
		std::vector<std::string> SubDir(const std::string& Expression);
		std::vector<std::wstring> SubDir(const std::wstring& Expression);
		std::vector<std::string> SubDirFull(const std::string& Expression);
		std::vector<std::wstring> SubDirFull(const std::wstring& Expression);
		int FileExists(const std::string& File);
		int FileExists(const std::wstring& File);
		void MakeDirectory(const std::string& Directory);
		void MakeDirectory(const std::wstring& Directory);

		JSON::Item LoadJson(const char* Filename);
		JSON::Item LoadJson(const wchar_t* Filename);
		std::wstring wCurrentPath();
		std::string CurrentPath();
		void CurrentPath(const std::string path);
		void CurrentPath(const std::wstring path);
		std::vector<std::string> FileExtensions(const std::string&);
		std::vector<std::wstring> FileExtensions(const std::wstring&);

		std::string FileExtension(const std::string&);
		std::wstring FileExtension(const std::wstring&);

		//std::string FilePath(const std::string &);
		//std::wstring FilePath(const std::wstring &);

		std::vector<std::string> SplitPath(const std::string& Path);
		std::vector<std::wstring> SplitPath(const std::wstring& Path);

		std::string ReplaceExtension(const std::string& Filename, const std::string& NewExtension);
		std::wstring ReplaceExtension(const std::wstring& Filename, const std::wstring& NewExtension);


#ifdef _WINDOWS
		std::string OpenFileDialog(HWND Parent, const char* Filter, const char* Filename = 0);
		std::wstring OpenFileDialog(HWND Parent, const wchar_t* Filter, const wchar_t* Filename = 0);
		std::string SaveFileDialog(HWND Parent, const char* Filter, const char* Filename = 0);
		std::wstring SaveFileDialog(HWND Parent, const wchar_t* Filter, const wchar_t* Filename = 0);
#endif
	};
	class BinWriter {
		FILE* fout;
	public:
		BinWriter(const std::string_view& Filename);
		BinWriter(const std::wstring_view& Filename);
		~BinWriter();
		int Write(void* data, size_t sz);
		template<typename T>
		int Write(const T& data) { return Write((void*)&data, sizeof(T)); }
		template<typename T>
		int Write(const std::vector<T>& data) { return Write((void*)&data[0], data.size() * sizeof(T)); }
	};
}

#endif