#ifndef __PVX_ENCODE_H__
#define __PVX_ENCODE_H__

#include<vector>
#include<string>
#include<array>
#include<type_traits>
#include<string_view>

namespace PVX{
	namespace Encode{
		std::string ToString(const std::vector<unsigned char> & data);
		std::string ToString(const std::wstring & data);
		std::wstring ToString(const std::string & data);
		std::string Base64(const void * data, size_t size);
		std::string Base64(const std::vector<unsigned char>& data);
		template<int Size>
		inline std::string Base64(const std::array<unsigned char, Size>& data) {
			return Base64(data.data(), Size);
		}
		std::string Base64Url(const void* data, size_t size, bool NoPadding = false);
		template<typename T>
		inline std::string Base64Url(const T& data, bool NoPadding = false) { return Base64Url(data.data(), data.size(), NoPadding); }
		inline std::string Base64Url(const char* data, bool NoPadding = false) { return Base64Url(data, strlen(data), NoPadding); }
		std::vector<unsigned char> UTF0(const std::wstring & Text);
		std::vector<unsigned char> UTF(const std::wstring & Text);
		void UTF(std::vector<unsigned char> & utf, const std::wstring & Text);
		size_t UTF(unsigned char * Data, const wchar_t * Str);
		size_t UTF_Length(const wchar_t * Str);
		size_t UTF_Length(const std::wstring & Str);
		size_t Uri_Length(const char * u);
		std::string UriEncode(const std::string & s);
		std::string UriEncode(const std::vector<unsigned char> & s);
		std::string Uri(const std::wstring & s);
		std::string Windows1253_Greek(const std::wstring & data);
		std::string ToHex(const std::vector<unsigned char>& Data);
		std::string ToHexUpper(const std::vector<unsigned char>& Data);
		std::wstring wToHex(const std::vector<unsigned char>& Data);
		std::wstring wToHexUpper(const std::vector<unsigned char>& Data);
	}
	namespace Decode{
		std::vector<unsigned char> Base64(const std::string & base64);
		std::vector<unsigned char> Base64(const std::wstring & base64);
		std::string Base64_String(const std::string & base64);

		std::vector<unsigned char> Base64Url(const char* Base64, size_t sz);
		std::vector<unsigned char> Base64Url(const std::string& base64);
		std::vector<unsigned char> Base64Url(const wchar_t* Base64, size_t sz);
		std::vector<unsigned char> Base64Url(const std::wstring& base64);

		std::wstring UTF(const unsigned char * utf, size_t sz);
		std::wstring UTF(const std::vector<unsigned char> & Utf);
		wchar_t * pUTF(const unsigned char * utf);
		std::wstring Windows1253(const std::vector<unsigned char> & s);
		std::wstring Windows1253(const char * s);
		std::wstring Uri(const std::string & s);
		std::wstring Uri(const std::wstring & s);
		std::wstring Unescape(const std::wstring& x);

		std::vector<unsigned char> FromHex(const std::string_view& str);
		std::vector<unsigned char> FromHex(const std::wstring_view& str);
	}
}

#endif