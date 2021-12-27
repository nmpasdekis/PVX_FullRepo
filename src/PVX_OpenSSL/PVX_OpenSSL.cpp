#include <PVX_OpenSSL.h>


#include <functional>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <openssl/ssl.h>
#include <openssl/err.h>



namespace {
	using SendCLB = int (*)(void* dt, const void*, size_t sz);
	using ReceiveCLB = int (*)(void* sock, void*, size_t sz);
	using ReleaseCLB = void (*)(void*);

	struct Callbacks {
		SendCLB Send;
		ReceiveCLB Recv;
		ReleaseCLB Release;
	};

	class socketData {
	public:
		SOCKET Socket;
		Callbacks* Actions;
		int Running;
		struct sockaddr Address;
		void* SSL;
	};

	Callbacks SSL_ClientActions{
		[](void* sock, const void* Data, size_t Size) {
			return SSL_write((SSL*)((socketData*)sock)->SSL, Data, (int)Size);
		},
		[](void* sock, void* Data, size_t Size) {
			return SSL_read((SSL*)((socketData*)sock)->SSL, Data, (int)Size);
		},
		[](void* sock) {
			SSL_shutdown((SSL*)((socketData*)sock)->SSL);
			SSL_free((SSL*)((socketData*)sock)->SSL);
			closesocket((SOCKET)((socketData*)sock)->Socket);
		}
	};

	class __initOpenSSL {
	public:
		__initOpenSSL() {
			OpenSSL_add_ssl_algorithms();
		}
		~__initOpenSSL() {
			EVP_cleanup();
		}
	} ___initOpenSSL;
}

namespace PVX::Security {
	OpenSSL::OpenSSL() : sslContext{ (void*)SSL_CTX_new(TLS_client_method()) } {}
	OpenSSL::OpenSSL(const char* Certificate, const char* Pass) {
		SSL_CTX* ssl = SSL_CTX_new(TLS_server_method());
		SSL_CTX_set_ecdh_auto(ssl, 1);
		SSL_CTX_use_certificate_file(ssl, Certificate, SSL_FILETYPE_PEM);
		if(Pass)
			SSL_CTX_use_PrivateKey_file(ssl, Pass, SSL_FILETYPE_PEM);
		else
			SSL_CTX_use_PrivateKey_file(ssl, Certificate, SSL_FILETYPE_PEM);

		sslContext = (void*)ssl;
	}
	OpenSSL::~OpenSSL() { SSL_CTX_free((SSL_CTX*)sslContext); }

	void OpenSSL::ConvertClientSocket(PVX::Network::TcpSocket& Socket) {
		auto& sock = Socket.GetInternalData<socketData>();
		sock.Actions = &SSL_ClientActions;
		SSL* ssl = SSL_new((SSL_CTX*)sslContext);
		sock.SSL = (void*)ssl;
		SSL_set_fd(ssl, (int)sock.Socket);
		SSL_connect(ssl);
	}

	void OpenSSL::ConvertServerSocket(PVX::Network::TcpSocket& Socket) {
		auto& sock = Socket.GetInternalData<socketData>();
		sock.Actions = &SSL_ClientActions;
		SSL* ssl = SSL_new((SSL_CTX*)sslContext);
		sock.SSL = (void*)ssl;
		SSL_set_fd(ssl, (int)sock.Socket);
		SSL_accept(ssl);
	}
}