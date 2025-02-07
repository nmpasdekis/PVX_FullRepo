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
#include<optional>

namespace PVX {
	namespace IO {
		constexpr wchar_t sep = std::filesystem::path::preferred_separator;
		constexpr wchar_t otherSep = [] { return sep == L'\\' ? L'/' : L'\\'; }();
		constexpr wchar_t sepString[2]{ sep, 0 };
		constexpr wchar_t otherSepString[2]{ otherSep, 0 };

		class ChangeTracker {
			std::filesystem::path Filename;
			void GetLastTime();
		public:
			std::filesystem::file_time_type LastTime;
			ChangeTracker(const std::filesystem::path& Filename);
			bool Changed();
			operator const std::filesystem::path& ();
		};

		class ChangeEventer {
			struct Events {
				ChangeTracker File;
				std::function<void(const std::filesystem::path&)> Do;
			};
			std::mutex Locker;
			bool Running, AutoRemove;
			std::unique_ptr<std::thread> Tracker;
			std::vector<Events> Files;
			std::atomic_bool Paused = false;
			std::function<void(const std::filesystem::path&)> defaultClb;
			std::function<void(const std::filesystem::path&)> onDeletedClb;
		public:
			ChangeEventer(bool Automatic, bool AutoRemove = false, std::function<void(const std::filesystem::path&)> defaulFunc = nullptr, std::function<void(const std::filesystem::path&)> onDelete = nullptr);
			~ChangeEventer();
			void Run();
			inline void Pause() { Paused = true; }
			inline void Start() { Paused = false; }
			bool Track(const std::filesystem::path& Filename, std::function<void(const std::filesystem::path&)> clb, bool RunFirstTime = true);
			bool Track(const std::filesystem::path& Filename, bool RunFirstTime = true);
		};

		std::wstring ReadUtf(const std::filesystem::path& Filename);
		int WriteUtf(const std::filesystem::path &Filename, const std::wstring &Text);

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

		inline void ReadLines(const std::filesystem::path& Filename, std::function<void(const std::string&)> clb) {
			std::ifstream fin(Filename.c_str());
			if (fin.fail())return;
			std::string line;
			while (!fin.eof()) {
				std::getline(fin, line);
				clb(line);
			}
		}

		inline void ReadLinesUTF(const std::filesystem::path& Filename, std::function<void(const std::wstring&)> clb) {
			std::ifstream fin(Filename.c_str());
			if (fin.fail())return;
			std::string line;
			while (!fin.eof()) {
				std::getline(fin, line);
				clb(PVX::Decode::UTF((unsigned char*)line.c_str(), line.size()));
			}
		}

		size_t FileSize(FILE* fin);
		size_t FileSize(const std::filesystem::path& filename);

		std::filesystem::path FindFileFullPath(const std::filesystem::path& Filename);

		std::filesystem::path FilePathPart(const std::filesystem::path& Filename);

		int Write(const std::filesystem::path& fn, const void* data, size_t Size);
		inline int Write(const std::filesystem::path& fn, const std::vector<uint8_t>& Data) {
			return Write(fn, Data.data(), Data.size());
		}
		int Write(const std::filesystem::path& fn, const PVX::JSON::Item& Data);

		std::vector<uint8_t> ReadBinary(const std::filesystem::path& Filename);
		std::vector<uint8_t> ReadBinary(const std::filesystem::path& Filename, size_t offset, size_t length);
		size_t ReadBinary(const std::filesystem::path& Filename, std::vector<uint8_t>& Data);
		size_t AppendBinary(const std::filesystem::path& Filename, std::vector<uint8_t>& Data);
		size_t ReadBinary(const std::filesystem::path& Filename, size_t offset, size_t length, std::vector<uint8_t>& Data);

		std::string ReadText(const std::filesystem::path& Filename);

		std::vector<std::filesystem::path> Dir(const std::filesystem::path& Expression);
		std::vector<std::filesystem::path> DirFull(const std::filesystem::path& Expression);
		std::vector<std::filesystem::path> SubDir(const std::filesystem::path& Expression);
		std::vector<std::filesystem::path> SubDirFull(const std::filesystem::path& Expression);

		int FileExists(const std::filesystem::path& File);
		void MakeDirectory(const std::filesystem::path& Directory);

		JSON::Item LoadJson(const std::filesystem::path& Filename);

		std::filesystem::path FileExtension(const std::filesystem::path& Filename);

		std::vector<std::string> SplitPath(const std::string& Path);
		std::vector<std::wstring> SplitPath(const std::wstring& Path);

		std::filesystem::path ReplaceExtension(const std::filesystem::path& Filename, const std::filesystem::path& NewExtension);


#ifdef _WINDOWS
		std::string OpenFileDialog(HWND Parent, const char* Filter, const char* Filename = 0);
		std::wstring OpenFileDialog(HWND Parent, const wchar_t* Filter, const wchar_t* Filename = 0);
		std::string SaveFileDialog(HWND Parent, const char* Filter, const char* Filename = 0);
		std::wstring SaveFileDialog(HWND Parent, const wchar_t* Filter, const wchar_t* Filename = 0);


		class PathChangeEventer {
		public:
			PathChangeEventer(std::filesystem::path dir, std::function<void(const std::filesystem::path &)> Callback, bool Start = true);
			~PathChangeEventer();
			void Do();
			void Start();
			void Track(const std::filesystem::path &Filename);
			std::mutex mtx;
		private:
			std::map<std::filesystem::path, std::filesystem::file_time_type> Files;
			HANDLE hDir;
			std::filesystem::path Path;
			std::function<void(const std::filesystem::path &)> Callback;
			std::optional<std::thread> th;
			bool Running = false;
		};

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