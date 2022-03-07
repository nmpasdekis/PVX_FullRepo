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
	OpenSSL::OpenSSL(int Type) : sslContext{ [](int i) {
		switch (i) {
			case -1: return (void*)SSL_CTX_new(TLS_method());
			case 0: return (void*)SSL_CTX_new(TLS_client_method());
			case 1: return (void*)SSL_CTX_new(SSLv23_client_method());
			//case 2: return (void*)SSL_CTX_new(SSLv3_client_method());
			//case 3: return (void*)SSL_CTX_new(TLSv1_client_method());
			//case 4: return (void*)SSL_CTX_new(TLSv1_1_client_method());
			//case 5: return (void*)SSL_CTX_new(TLSv1_2_client_method());
			case 6: return (void*)SSL_CTX_new(DTLS_client_method());
			//case 7: return (void*)SSL_CTX_new(DTLSv1_client_method());
			//case 8: return (void*)SSL_CTX_new(DTLSv1_2_client_method());
			default: return (void*)SSL_CTX_new(TLS_client_method());
		}
	}(Type) } {
	
		//int err = SSL_CTX_use_certificate_file((SSL_CTX*)sslContext, "cacert.pem", SSL_FILETYPE_PEM);
		//err = SSL_CTX_use_PrivateKey_file((SSL_CTX*)sslContext, "cacert.pem", SSL_FILETYPE_PEM);
		//err++;
	}
	OpenSSL::OpenSSL() : sslContext{ (void*)SSL_CTX_new(TLS_client_method()) } {

	}

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
		SSL_set_connect_state(ssl);
		sock.SSL = (void*)ssl;
		int err = SSL_set_fd(ssl, (int)sock.Socket);
		if (err<0) {
			auto err2 = SSL_get_error(ssl, err);
			char errString[1024];
			ERR_error_string(err2, errString);
			err2++;
		}
		err = SSL_connect(ssl);
		if (err<0) {
			auto err2 = SSL_get_error(ssl, err);
			char errString[1024];
			ERR_error_string(err2, errString);

			int err3 = WSAGetLastError();

			err2 = SSL_ERROR_NONE;
		}
	}
	enum class SSLErrors {
		ERROR_NONE,
		ERROR_SSL,
		ERROR_WANT_READ,
		ERROR_WANT_WRITE,
		ERROR_WANT_X509_LOOKUP,
		ERROR_SYSCALL,
		ERROR_ZERO_RETURN,
		ERROR_WANT_CONNECT,
		ERROR_WANT_ACCEPT,
		ERROR_WANT_ASYNC,
		ERROR_WANT_ASYNC_JOB,
		ERROR_WANT_CLIENT_HELLO_CB,
		ERROR_WANT_RETRY_VERIFY
	};

	void OpenSSL::ConvertClientSocket(PVX::Network::TcpSocket& Socket, const char* HostName) {
		auto& sock = Socket.GetInternalData<socketData>();
		sock.Actions = &SSL_ClientActions;
		SSL* ssl = SSL_new((SSL_CTX*)sslContext);
		SSL_set_connect_state(ssl);
		SSL_set_tlsext_host_name(ssl, HostName);
		sock.SSL = (void*)ssl;
		int err = SSL_set_fd(ssl, (int)sock.Socket);
		if (err<0) {
			auto err2 = (SSLErrors)SSL_get_error(ssl, err);
			char errString[1024];
			ERR_error_string((int)err2, errString);
		}
		err = SSL_connect(ssl);
		if (err<0) {
			auto err2 = (SSLErrors)SSL_get_error(ssl, err);
			char errString[1024];
			ERR_error_string((int)err2, errString);

			int err3 = WSAGetLastError();

			err3 = SSL_ERROR_NONE;
		}
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