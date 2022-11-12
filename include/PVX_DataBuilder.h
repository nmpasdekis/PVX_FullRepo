#ifndef __PVX_DATA_BUILDER_H__
#define __PVX_DATA_BUILDER_H__

#include <string>
#include <vector>
#include <PVX_json.h>
#include <filesystem>


class PVX_DataBuilder{
public:
	size_t GetLength();
	unsigned char * GetData();
	void SetData(const void * Data, int Size);
	void SetData(const std::vector<unsigned char> & Data);
	int BinaryFile(const std::filesystem::path& Filename);
	void AppendBinaryFile(const std::filesystem::path& Filename);
	void WriteUTF(const std::wstring & String);
	void WriteText(const std::string & String);
	//void AppendUTF(const std::wstring & String);
	//void WriteUTF(const std::string & String);
	//void AppendUTF(const std::string & String);
	void Clear(int BOM = 0);
	PVX_DataBuilder & operator<<(const std::wstring & str);
	PVX_DataBuilder & operator<<(const std::string & str);
	PVX_DataBuilder & operator<<(const std::vector<unsigned char> & data);
	PVX_DataBuilder & operator<<(const unsigned char* RawString);
	PVX_DataBuilder & operator<<(int64_t Number);
	std::vector<uint8_t> & GetDataVector();
protected:
	std::vector<uint8_t> Data;
};

#endif