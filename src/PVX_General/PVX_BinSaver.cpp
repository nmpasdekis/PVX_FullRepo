#include <PVX_BinSaver.h>
#include <PVX_Encode.h>

namespace PVX {
	BinSaver::BinSaver(const char* Filename, const char* head) : 
		fout{ Filename, std::ofstream::binary } 
	{
		if (!fout.is_open())return;
		Begin(head);
	}

	BinSaver::BinSaver(const wchar_t* Filename, const char* head):
		fout{ (const char*)PVX::Encode::UTF0(Filename).data(), std::ofstream::binary }
	{
		if (!fout.is_open())return;
		Begin(head);
	}

	BinSaver::BinSaver(const std::string& Filename) :BinSaver(Filename.c_str(), "PVXB") {}
	BinSaver::BinSaver(const char* Filename) : BinSaver(Filename, "PVXB") {}

	void BinSaver::Begin(const char* Name) {
		fout.write(Name, 4);
		SizePos.push_back(fout.tellp());
		fout.write(Name, 4);
	}

	size_t BinSaver::write(const void* buffer, size_t size, size_t count) {
		fout.write((const char*)buffer, size * count);
		return count;
	}

	size_t BinSaver::write(const std::vector<unsigned char>& Bytes) {
		fout.write((const char*)Bytes.data(), Bytes.size());
		return Bytes.size();
	}

	int BinSaver::OK() {
		return fout.is_open();
	}

	void BinSaver::End() {
		auto cur = fout.tellp();
		auto pos = SizePos[SizePos.size() - 1];
		SizePos.pop_back();
		fout.seekp(pos);
		pos = cur - pos - 4;
		fout.write((const char*)&pos, 4);
		fout.seekp(cur);
	}

	int BinSaver::Save() {
		End();
		return SizePos.size() == 0;
	}

	BinSaver::~BinSaver() {
		if (fout) Save();
	}

	BinLoader::BinLoader(std::ifstream& Fin, size_t Size, BinLoader* Parent):
		fin{ Fin }
	{
		this->Size = Size;
		this->Parent = Parent;
		cur = 0;
	}
	BinLoader::BinLoader(const char* fn, const char* header):
		__fin(fn, std::ifstream::binary), fin{ __fin }
	{
		BinHeader hd;
		if (fin.is_open()) {
			fin.read((char*)&hd, sizeof(BinHeader));
			cur = fin.tellg();
			if (cur == sizeof(BinHeader) && hd.iName == (*(int*)header)) {
				Size = hd.Size;
			}
		}
		Parent = 0;
	}
	BinLoader::BinLoader(const wchar_t* fn, const char* header):
#ifndef __linux
		__fin(fn, std::ifstream::binary), fin{ __fin }
#else
		__fin((const char*)PVX::Encode::UTF0(fn).data(), std::ifstream::binary), fin{ __fin }
#endif
	{
		BinHeader hd;
		if (fin.is_open()) {
			fin.read((char*)&hd, sizeof(BinHeader));
			cur = fin.tellg();
			if (cur == sizeof(BinHeader) && hd.iName == (*(int*)header)) {
				Size = hd.Size;
			}
		}
		Parent = 0;
	}
	BinLoader::BinLoader(const std::string& fn) :BinLoader(fn.c_str(), "PVXB") {}
	BinLoader::BinLoader(const char* fn) : BinLoader(fn, "PVXB") {}
	BinLoader::~BinLoader() {
		if (cur < Size)
			Execute();
		if (Parent) {
			Parent->cur += Size;
		}
	}
	void BinLoader::Process(const char* header, std::function<void(BinLoader&)> Loader) {
		this->Loader[*(unsigned int*)header] = Loader;
	}
	void BinLoader::ProcessAny(std::function<void(BinLoader&, const char*)> Loader) {
		this->AnyLoader = Loader;
	}
	void BinLoader::Read(void* Data, size_t sz) {
		fin.read((char*)Data, sz);
		cur += fin.gcount();
		//cur += fread_s(Data, sz, 1, sz, fin);
	}
	size_t BinLoader::ReadAll(void* Data) {
		fin.read((char*)Data, Size - cur);
		auto sz = fin.gcount();
		cur = Size;
		return sz;
	}
	std::vector<unsigned char> BinLoader::ReadAll() {
		std::vector<unsigned char> ret(Size - cur);
		Read(&ret[0], (int)(Size - cur));
		return ret;
	}
	void BinLoader::Execute() {
		BinHeader hd;
		while (cur < Size) {
			fin.read((char*)&hd, sizeof(BinHeader));
			cur += fin.gcount();
			if (Loader.find(hd.iName) != Loader.end()) {
				BinLoader bl(fin, hd.Size, this);
				Loader[hd.iName](bl);
			} else if (AnyLoader != nullptr) {
				BinLoader bl(fin, hd.Size, this);
				AnyLoader(bl, hd.sName);
			} else {
				fin.seekg(cur + hd.Size);
				cur += hd.Size;
			}
		}
	}
	int BinLoader::OK() {
		return fin.is_open();
	}

	size_t BinLoader::Remaining(int ItemSize) {
		return (Size - cur) / ItemSize;
	}

	std::string BinLoader::RemainingAsString() {
		std::string ret;
		ret.resize(Remaining());
		ReadAll(&ret[0]);
		return ret;
	}
	std::wstring BinLoader::RemainingAsWideString() {
		std::wstring ret;
		ret.resize(Remaining() >> 1);
		ReadAll(&ret[0]);
		return ret;
	}

	void BinLoader::ReadBytes(std::vector<unsigned char>& Bytes, size_t size) {
		if (!size) size = Remaining();
		Bytes.resize(size);
		Read(&Bytes[0], size);
	}
}