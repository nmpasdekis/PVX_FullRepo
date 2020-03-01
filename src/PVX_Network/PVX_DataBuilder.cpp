#include <PVX_DataBuilder.h>
#include <PVX_Encode.h>
#include <PVX_File.h>
#include <PVX_json.h>


PVX_DataBuilder & PVX_DataBuilder::operator<<(const std::wstring & str){
	size_t start = Data.size();
	size_t sz = PVX::Encode::UTF_Length(str);
	if (sz) {
		Data.resize(start + sz);
		PVX::Encode::UTF(&Data[start], str.c_str());
	}
	return *this;
}
PVX_DataBuilder & PVX_DataBuilder::operator<<(const std::vector<unsigned char> & data){
	size_t start = Data.size();
	size_t sz = data.size();
	if (sz) {
		Data.resize(start + sz);
		memcpy(&Data[start], data.data(), sz);
	}
	return *this;
}

PVX_DataBuilder & PVX_DataBuilder::operator<<(const unsigned char * RawString) {
	int len;
	for (len = 0; RawString[len]; len++);
	size_t start = Data.size();
	Data.resize(start + len);
	memcpy(&Data[start], RawString, len);
	return *this;
}

PVX_DataBuilder & PVX_DataBuilder::operator<<(long long Number) {
	char RawString[128];
	_i64toa_s(Number, RawString, 128, 10);
	int len;
	for (len = 0; RawString[len]; len++);
	size_t start = Data.size();
	Data.resize(start + len);
	memcpy(&Data[start], RawString, len);
	return *this;
}

PVX_DataBuilder & PVX_DataBuilder::operator<<(const std::string & Str){
	auto start = Data.size();
	std::wstring str = PVX::Decode::Windows1253(Str.c_str());
	int sz = PVX::Encode::UTF_Length(str);
	Data.resize(start + sz);
	PVX::Encode::UTF(&Data[start], str.c_str());
	return *this;
}

size_t PVX_DataBuilder::GetLength(){
	return Data.size();
}
unsigned char * PVX_DataBuilder::GetData(){
	return Data.data();
}
std::vector<unsigned char> & PVX_DataBuilder::GetDataVector(){
	return Data;
}
void PVX_DataBuilder::SetData(const void * Data, int Size){
	this->Data.resize(Size);
	memcpy(this->Data.data(), Data, Size);
}
void PVX_DataBuilder::SetData(const std::vector<unsigned char>& Data) {
	this->Data = Data;
}
void PVX_DataBuilder::Clear(int BOM){
	Data.clear();
	if (BOM){
		Data.resize(3);
		Data[0] = 0xef;
		Data[1] = 0xbb;
		Data[2] = 0xbf;
	}
}
void PVX_DataBuilder::WriteUTF(const std::wstring & String){
	Data = PVX::Encode::UTF(String);
}
void PVX_DataBuilder::WriteText(const std::string & String) {
	Data.resize(String.size());
	memcpy(&Data[0], &String[0], String.size());
}
int PVX_DataBuilder::BinaryFile(const char * fn){
	Data = PVX::IO::ReadBinary(fn);
	return 200;
}
int PVX_DataBuilder::BinaryFile(const wchar_t * fn){
	FILE * fin;
	_wfopen_s(&fin, fn, L"rb");
	if (!fin)return 404;
	fseek(fin, 0, SEEK_END);
	int Size = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	this->Data.resize(Size);
	fread(Data.data(), 1, Size, fin);

	fclose(fin);
	return 200;
}
void PVX_DataBuilder::AppendBinaryFile(const char * fn){
	FILE * fin;
	fopen_s(&fin, fn, "rb");
	if (!fin)return;
	fseek(fin, 0, SEEK_END);
	int Size = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	auto sz = Data.size();
	this->Data.resize(Size + sz);
	fread(Data.data()+sz, 1, Size, fin);
	fclose(fin);
}