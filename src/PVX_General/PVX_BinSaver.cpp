#include <PVX_BinSaver.h>

namespace PVX {
	BinSaver::BinSaver(const char* Filename, const char* head) {
		if (fopen_s(&fout, Filename, "wb"))return;
		Begin(head);
	}

	BinSaver::BinSaver(const wchar_t* Filename, const char* head) {
		if (_wfopen_s(&fout, Filename, L"wb"))return;
		Begin(head);
	}

	BinSaver::BinSaver(const std::string& Filename) :BinSaver(Filename.c_str(), "PVXB") {}
	BinSaver::BinSaver(const char* Filename) : BinSaver(Filename, "PVXB") {}

	void BinSaver::Begin(const char* Name) {
		fwrite(Name, 1, 4, fout);
		SizePos.push_back(ftell(fout));
		fwrite(Name, 1, 4, fout);
	}

	size_t BinSaver::write(const void* buffer, size_t size, size_t count) {
		return fwrite(buffer, size, count, fout);
	}

	size_t BinSaver::write(const std::vector<unsigned char>& Bytes) {
		return write(&Bytes[0], 1, Bytes.size());
	}

	int BinSaver::OK() {
		return fout != NULL;
	}

	void BinSaver::End() {
		long cur = ftell(fout);
		long pos = SizePos[SizePos.size() - 1];
		SizePos.pop_back();
		fseek(fout, pos, SEEK_SET);
		pos = cur - pos - 4;
		fwrite(&pos, 4, 1, fout);
		fseek(fout, cur, SEEK_SET);
	}

	int BinSaver::Save() {
		End();
		fclose(fout);
		fout = 0;
		return SizePos.size() == 0;
	}

	BinSaver::~BinSaver() {
		if (fout) Save();
	}

	BinLoader::BinLoader(FILE* fin, size_t Size, BinLoader* Parent) {
		this->fin = fin;
		this->Size = Size;
		this->Parent = Parent;
		cur = 0;
	}
	BinLoader::BinLoader(const char* fn, const char* header) {
		fopen_s(&fin, fn, "rb");
		BinHeader hd;
		if (fin) {
			cur = fread_s(&hd, sizeof(BinHeader), 1, sizeof(BinHeader), fin);
			if (cur == sizeof(BinHeader) && hd.iName == (*(int*)header)) {
				Size = hd.Size;
			}
		}
		Parent = 0;
	}
	BinLoader::BinLoader(const wchar_t* fn, const char* header) {
		_wfopen_s(&fin, fn, L"rb");
		BinHeader hd;
		if (fin) {
			int sz = sizeof(BinHeader);
			cur = fread_s(&hd, sizeof(BinHeader), 1, sizeof(BinHeader), fin);
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
		} else
			fin&& fclose(fin);
	}
	void BinLoader::Process(const char* header, std::function<void(BinLoader&)> Loader) {
		this->Loader[*(unsigned int*)header] = Loader;
	}
	void BinLoader::ProcessAny(std::function<void(BinLoader&, const char*)> Loader) {
		this->AnyLoader = Loader;
	}
	void BinLoader::Read(void* Data, size_t sz) {
		cur += fread_s(Data, sz, 1, sz, fin);
	}
	size_t BinLoader::ReadAll(void* Data) {
		auto sz = fread_s(Data, Size - cur, 1, Size - cur, fin);
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
			cur += fread_s(&hd, sizeof(BinHeader), 1, sizeof(BinHeader), fin);
			if (Loader.find(hd.iName) != Loader.end()) {
				BinLoader bl(fin, hd.Size, this);
				Loader[hd.iName](bl);
			} else if (AnyLoader != nullptr) {
				BinLoader bl(fin, hd.Size, this);
				AnyLoader(bl, hd.sName);
			} else {
				fseek(fin, hd.Size, SEEK_CUR);
				cur += hd.Size;
			}
		}
	}
	int BinLoader::OK() {
		return fin != 0;
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