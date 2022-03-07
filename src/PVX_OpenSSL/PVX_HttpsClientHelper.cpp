#include <PVX_HttpsClient.h>

namespace {
	PVX::Security::OpenSSL clientSSL(-1);
}

namespace PVX::Network {
	PVX::Network::HttpClient MakeHttpClient(const std::wstring& Domain) {
		auto ret = PVX::Network::HttpClient(Domain);
		auto dom = ret.Domain();
		if (ret.Protocol()=="http")
			return ret;
		return ret.OnConnect([dom](PVX::Network::TcpSocket& s) {
			s.SetOption(TcpSocketOption::KeepAlive, 1);
			clientSSL.ConvertClientSocket(s, dom.c_str());
		});
	}
}