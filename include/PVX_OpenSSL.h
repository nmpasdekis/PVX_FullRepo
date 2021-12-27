#pragma once

#include <PVX_Network.h>


namespace PVX::Security {
	class OpenSSL {
		void* sslContext;
	public:
		OpenSSL();
		OpenSSL(const char* Certificate, const char* Pass = nullptr);
		~OpenSSL();
		void ConvertClientSocket(PVX::Network::TcpSocket& Socket);
		void ConvertServerSocket(PVX::Network::TcpSocket& Socket);
	};
}