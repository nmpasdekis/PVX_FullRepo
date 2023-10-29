#include <PVX_Network.h>
#include <PVX_OpenSSL.h>

namespace PVX::Network {
	PVX::Network::HttpClient MakeHttpClient(const std::wstring& Domain);
	PVX::Network::HttpClient MakeHttpClient(const std::string& Domain);
}