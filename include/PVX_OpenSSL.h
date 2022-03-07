#pragma once

#include <PVX_Network.h>


namespace PVX::Security {
	class OpenSSL {
		void* sslContext;
	public:
		/* 
		0: TLS
		1: SSLv23
		// 2: SSLv3
		3: TLSv1
		4: TLSv1.1
		5: TLSv1.2
		6: DTLS
		7: DTLSv1
		8: DTLSv1.2
		*/
		OpenSSL(int Type);
		OpenSSL();
		OpenSSL(const char* Certificate, const char* Pass = nullptr);
		~OpenSSL();
		void ConvertClientSocket(PVX::Network::TcpSocket& Socket);
		void ConvertClientSocket(PVX::Network::TcpSocket& Socket, const char* HostName);
		void ConvertServerSocket(PVX::Network::TcpSocket& Socket);
	};
}