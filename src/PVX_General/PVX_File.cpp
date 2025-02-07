#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include<vector>
#include<string>
#include<PVX_File.h>
//#include<stdio.h>
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

namespace PVX {
	namespace IO {
		namespace fs = std::filesystem;

		void ChangeTracker::GetLastTime() {
			LastTime = fs::directory_entry(Filename).last_write_time();
		}
		ChangeTracker::ChangeTracker(const std::filesystem::path& Filename) :
			Filename{ Filename },
			LastTime{ fs::exists(Filename) ? fs::directory_entry(Filename).last_write_time() : fs::file_time_type{} } {}
		bool ChangeTracker::Changed() {
			auto lt = LastTime;
			GetLastTime();
			return lt != LastTime;
		}
		ChangeTracker::operator const std::filesystem::path& () {
			return Filename;
		}
		ChangeEventer::ChangeEventer(bool Automatic, bool ar, std::function<void(const std::filesystem::path&)> defaulFunc, std::function<void(const std::filesystem::path&)> deleteFunc) :
			Running{ Automatic }, Tracker{ Automatic ? new std::thread([this]() {
				while (Running) {
					if (!Paused) Run();
					std::this_thread::sleep_for(std::chrono::milliseconds(333));
				}
			}) : nullptr }, AutoRemove{ ar }, defaultClb{ defaulFunc }, onDeletedClb{ deleteFunc }
		{}
		void ChangeEventer::Run() {
			std::unique_lock<std::mutex> lock{ Locker };
			std::vector<size_t> toDelete;
			size_t i = 0;
			for (auto& t : Files) {
				if (fs::exists(t.File)) {
					auto dbg = t.File.LastTime;
					if (t.File.Changed()) 
						t.Do(t.File);
				}
				else {
					if (onDeletedClb) onDeletedClb(t.File);
					if (AutoRemove) toDelete.push_back(i);
				}
				if (Paused) break;
				i++;
			}
			if (AutoRemove) for (int64_t c = toDelete.size() - 1; c >= 0; c--) {
				Files.erase(Files.begin() + c);
			}
		}
		ChangeEventer::~ChangeEventer() {
			Running = 0;
			if(Tracker) Tracker->join();
		}
		bool ChangeEventer::Track(const std::filesystem::path& Filename, std::function<void(const std::filesystem::path&)> clb, bool run) {
			std::unique_lock<std::mutex> lock{ Locker };
			Events& e = Files.emplace_back(Events{ Filename, clb });
			if (e.File.Changed() && run) clb(Filename);
			return (fs::exists(Filename));
		}
		bool ChangeEventer::Track(const std::filesystem::path& Filename, bool run) {
			std::unique_lock<std::mutex> lock{ Locker };
			Events& e = Files.emplace_back(Events{ Filename, defaultClb });
			if (e.File.Changed() && run) defaultClb(Filename);
			return (fs::exists(Filename));
		}

		void MakeDirectory(const std::filesystem::path& Directory) {
			fs::create_directories(Directory);
		}

		std::wstring ReadUtf(const std::filesystem::path& Filename) {
			auto bin = ReadBinary(Filename.c_str());
			return PVX::Decode::UTF(bin);
		}
		int WriteUtf(const std::filesystem::path &Filename, const std::wstring &Text) {
			return Write(Filename, PVX::Encode::UTF(Text));
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
		size_t FileSize(const std::filesystem::path& filename) {
			return std::filesystem::file_size(filename);
		}
		std::filesystem::path FindFileFullPath(const std::filesystem::path& Filename) {
			namespace fs = std::filesystem;
			if (Filename.is_absolute()) return Filename;
			auto cp = fs::current_path() / Filename;
			if (std::filesystem::exists(cp)) return cp;
#ifdef __linux
			std::string_view var = getenv("PATH");
#else
			std::wstring_view var = _wgetenv(L"PATH");
#endif
			while (var.size()) {
				auto x = var.find(';'); if (x==std::string_view::npos) x = var.size();
				auto pp = fs::path{ var.substr(0, x) } / Filename;
				if (std::filesystem::exists(pp))
					return pp;
				var.remove_prefix(x+1);
			}
			return {};
		}

		std::filesystem::path FilePathPart(const std::filesystem::path& Filename) {
			return FindFileFullPath(Filename).remove_filename();
		}

		int Write(const std::filesystem::path& fn, const void* data, size_t Size) {
			std::ofstream fout(fn, std::ofstream::binary);
			if (fout.fail()) return 0;
			fout.write((const char*)data, Size);
			return 1;
		}

		std::vector<uint8_t> ReadBinary(const std::filesystem::path& Filename) {
			auto fl = std::ifstream(Filename, std::ifstream::binary | std::ifstream::ate);
			if (fl.fail()) return {};
			std::vector<uint8_t> ret(fl.tellg());
			fl.seekg(0);
			fl.read((char*)ret.data(), ret.size());
			return ret;
		}
		std::vector<unsigned char> ReadBinary(const std::filesystem::path& Filename, size_t offset, size_t length) {
			if (std::filesystem::exists(Filename)) {
				length = std::min(length, size_t(std::filesystem::file_size(Filename) - offset));
				std::vector<uint8_t> ret(length);
				auto fin = std::ifstream(Filename, std::ifstream::binary);
				fin.seekg(offset);
				fin.read((char*)ret.data(), ret.size());
				return ret;
			}
			return {};
		}
		size_t ReadBinary(const std::filesystem::path& Filename, std::vector<uint8_t>& Data) {
			if (std::filesystem::exists(Filename)) {
				Data.resize(std::filesystem::file_size(Filename));
				std::ifstream(Filename, std::ifstream::binary).read((char*)Data.data(), Data.size());
				return Data.size();
			}
			return 0;
		}
		size_t AppendBinary(const std::filesystem::path& Filename, std::vector<uint8_t>& Data) {
			if (std::filesystem::exists(Filename)) {
				auto sz = Data.size();
				auto nsz = std::filesystem::file_size(Filename);
				Data.resize(sz + nsz);
				std::ifstream(Filename, std::ifstream::binary).read((char*)Data.data() + sz, nsz);
				return nsz;
			}
			return 0;
		}
		size_t ReadBinary(const std::filesystem::path& Filename, size_t offset, size_t length, std::vector<unsigned char>& Data) {
			if (std::filesystem::exists(Filename)) {
				length = std::min(length, size_t(std::filesystem::file_size(Filename) - offset));
				Data.resize(length);
				auto fin = std::ifstream(Filename, std::ifstream::binary);
				fin.seekg(offset);
				fin.read((char*)Data.data(), Data.size());
				return Data.size();
			}
			return 0;
		}

		std::string ReadText(const std::filesystem::path& Filename) {
			std::ifstream inp(Filename);
			if(inp.fail())return "";
			return std::string(std::istreambuf_iterator<char>(inp), (std::istreambuf_iterator<char>()));
		}

		std::vector<std::string> SplitPath(const std::string & Path) {
			return PVX::String::Split_No_Empties(Path, "\\");
		}

		std::vector<std::wstring> SplitPath(const std::wstring & Path) {
			return PVX::String::Split_No_Empties(Path, L"\\");
		}

		std::filesystem::path ReplaceExtension(const std::filesystem::path& Filename, const std::filesystem::path& NewExtension) {
			std::filesystem::path ret = Filename;
			return ret.replace_extension(NewExtension);
		}

		JSON::Item LoadJson(const std::filesystem::path& Filename) {
			return JSON::parse(PVX::IO::ReadBinary(Filename));
		}
		int Write(const std::filesystem::path& Filename, const PVX::JSON::Item& Json) {
			return Write(Filename, PVX::Encode::UTF(PVX::JSON::stringify(Json)));
		}

		std::filesystem::path FileExtension(const std::filesystem::path& Filename) {
			return Filename.extension();
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
		std::vector<std::filesystem::path> Dir(const std::filesystem::path& filter) {
			std::vector<std::filesystem::path> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (!entry.is_directory()) {
				ret.push_back(entry.path().filename());
			}
			return ret;
		}
		std::vector<std::filesystem::path> DirFull(const std::filesystem::path& filter) {
			std::vector<std::filesystem::path> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (!entry.is_directory())
				ret.push_back(entry.path());
			return ret;
		}
		std::vector<std::filesystem::path> SubDir(const std::filesystem::path& filter) {
			std::vector<std::filesystem::path> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (entry.is_directory()) {
				ret.push_back(entry.path().filename());
			}
			return ret;
		}
		std::vector<std::filesystem::path> SubDirFull(const std::filesystem::path& filter) {
			std::vector<std::filesystem::path> ret;
			for (const auto& entry : fs::directory_iterator(filter)) if (entry.is_directory())
				ret.push_back(entry.path());
			return ret;
		}


		int FileExists(const fs::path& file) {
			return fs::exists(file.c_str());
		}
		int FileExists(const std::wstring& file) {
			return fs::exists(file.c_str());
		}
		int FileExists(const std::string& file) {
			return fs::exists(file.c_str());
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