#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>

#include<PVX_Network.h>


//#include<OpenSSl/ssl.h>
//#include<OpenSSl/err.h>
#include<stdio.h>
#include<mutex>

//#include <mstcpip.h>
//#include <rpc.h>
//#include <ntdsapi.h>


#include <wincrypt.h>
#include <schnlsp.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "fwpuclnt.lib")


//#ifdef _DEBUG
//#pragma comment (lib, "libcryptoMDd.lib")
//#pragma comment (lib, "libsslMDd.lib")
//#else
//#pragma comment (lib, "libcryptoMD.lib")
//#pragma comment (lib, "libsslMD.lib")
//#endif

namespace PVX {
	namespace Network {
		static int PVX_Socket_RefCount;
		static WSAData Windows_Socket_Data;
		struct PVX_SocketData {
			int Ref;
			SOCKET Socket;
			//SSL * SSL_Socket;
			int Running;
			struct sockaddr Address;
		};

//		static SSL_CTX * InitOpenSSL(const char* CertFile, const char* KeyFile) {
//			SSL_CTX *ctx;
//
////#if OPENSSL_VERSION_NUMBER < 0x10100000L
////			SSL_library_init();
////#else
////			OPENSSL_init_ssl(0, NULL);
////#endif
////			OpenSSL_add_all_algorithms();
////			SSL_load_error_strings();
//
//			const SSL_METHOD *method = TLSv1_server_method();// SSLv23_server_method;//
//			ctx = SSL_CTX_new(method);
//			if (ctx == NULL) {
//				ERR_print_errors_fp(stderr);
//				abort();
//			}
//		
//			if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0) {
//				ERR_print_errors_fp(stderr);
//				abort();
//			}
//			/* set the private key from KeyFile (may be the same as CertFile) */
//			if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0) {
//				ERR_print_errors_fp(stderr);
//				abort();
//			}
//			/* verify private key */
//			if (!SSL_CTX_check_private_key(ctx)) {
//				fprintf(stderr, "Private key does not match the public certificate\n");
//				abort();
//			}
//			return ctx;
//		}

		static SOCKET CreateListenSocket(const char * sPort) {
			struct addrinfo hint = { 0 };
			struct addrinfo *Source = 0;

			hint.ai_family = AF_INET;
			hint.ai_protocol = IPPROTO_TCP;
			hint.ai_socktype = SOCK_STREAM;
			hint.ai_flags = AI_PASSIVE;

			getaddrinfo(0, sPort, &hint, &Source);

			SOCKET SocketID = socket(Source->ai_family, Source->ai_socktype, Source->ai_protocol);
			if(SocketID == INVALID_SOCKET)
				return 0;
			if(bind(SocketID, Source->ai_addr, (int)Source->ai_addrlen) == SOCKET_ERROR)
				return 0;

			freeaddrinfo(Source);
			listen(SocketID, SOMAXCONN);
			return SocketID;
		}

		//OpenSSL::OpenSSL(): Data{ 0 } {}
		//OpenSSL::~OpenSSL() {
		//	if (Data)SSL_CTX_free((SSL_CTX*)Data);
		//}
		//OpenSSL::OpenSSL(const char * Certificate) {
		//	SetCertificate(Certificate);
		//}
		//OpenSSL::OpenSSL(const char * Certificate, const char * Pass) {
		//	SetCertificate(Certificate, Pass);
		//}
		//void OpenSSL::SetCertificate(const char * Certificate) {
		//	Data = InitOpenSSL(Certificate, Certificate);
		//}
		//void OpenSSL::SetCertificate(const char * Certificate, const char * Pass) {
		//	Data = InitOpenSSL(Certificate, Pass);
		//}
		static int next;

		TcpSocket::TcpSocket(const TcpSocket & s) {
			PVX_Socket_RefCount++;
			Data = s.Data;
			((PVX_SocketData*)Data)->Ref++;
		}
		TcpSocket & Network::TcpSocket::operator=(const TcpSocket & s) {
			PVX_Socket_RefCount++;
			DeRef();
			Data = s.Data;
			((PVX_SocketData*)Data)->Ref++;
			return *this;
		}
		std::string TcpSocket::RemoteAddress() const {
			auto& addr = ((PVX_SocketData*)Data)->Address;
			return 
				std::to_string(addr.sa_data[2]) + "." +
				std::to_string(addr.sa_data[3]) + "." +
				std::to_string(addr.sa_data[4]) + "." +
				std::to_string(addr.sa_data[5]);
		}
		std::wstring TcpSocket::wRemoteAddress() const {
			auto& addr = ((PVX_SocketData*)Data)->Address;
			return
				std::to_wstring(addr.sa_data[2]) + L"." +
				std::to_wstring(addr.sa_data[3]) + L"." +
				std::to_wstring(addr.sa_data[4]) + L"." +
				std::to_wstring(addr.sa_data[5]);
		}
		//TcpSocket::TcpSocket(const SOCKET s, const sockaddr & addr) {
		//	PVX_Socket_RefCount++;
		//	Data = new PVX_SocketData{ 1, s, 1, addr };
		//}
		TcpSocket::TcpSocket(const SOCKET_type s, const void* addr) {
			PVX_Socket_RefCount++;
			Data = new PVX_SocketData{ 1, (SOCKET)s, 1, *(sockaddr*)addr };
		}
		TcpSocket::TcpSocket() {
			if(!PVX_Socket_RefCount++)
				WSAStartup(MAKEWORD(2, 2), &Windows_Socket_Data);
			Data = new PVX_SocketData{ 1, socket(AF_INET, SOCK_STREAM, 0), 1, 0 };
		}
		TcpSocket::TcpSocket(const char * Port) {
			if(!PVX_Socket_RefCount++)
				WSAStartup(MAKEWORD(2, 2), &Windows_Socket_Data);
			Data = new PVX_SocketData{ 1, CreateListenSocket(Port), 1, 0 };
		}
		int TcpSocket::Send(const void * data, size_t sz) {
			//if (Normal)
			return send(((PVX_SocketData*)Data)->Socket, (const char*)data, (int)sz, 0);
			//return SSL_write(((PVX_SocketData*)Data)->SSL_Socket, data, (int)sz);
		}
		int TcpSocket::Send(const std::vector<unsigned char>& data) {
			//if (Normal)
			return send(((PVX_SocketData*)Data)->Socket, (const char*)data.data(), (int)data.size(), 0);
			//return SSL_write(((PVX_SocketData*)Data)->SSL_Socket, data.data(), (int)data.size());
		}
		int TcpSocket::Send(const std::string & data) {
			//if (Normal)
			return send(((PVX_SocketData*)Data)->Socket, (const char*)data.data(), (int)data.size(), 0);
			//return SSL_write(((PVX_SocketData*)Data)->SSL_Socket, data.data(), (int)data.size());
		}
		size_t TcpSocket::SendFragmented(const std::vector<unsigned char> & data, size_t FragmentSize) {
			size_t Offset = 0;
			size_t sz = data.size();
			size_t SendSize = FragmentSize;
			while (Offset + FragmentSize < sz && SendSize == FragmentSize) {
				SendSize = send(((PVX_SocketData*)Data)->Socket, (const char*)&data[Offset], FragmentSize, 0);
				Offset += SendSize;
			}
			if (Offset < FragmentSize && SendSize >= 0) {
				SendSize = send(((PVX_SocketData*)Data)->Socket, (const char*)&data[Offset], (int)(data.size() - Offset), 0);
				Offset += SendSize;
			}
			return Offset;
		}
		int TcpSocket::Receive(void * data, size_t Size) {
			//if (Normal)
			return recv(((PVX_SocketData*)Data)->Socket, (char*)data, (int)Size, 0);
			//return SSL_read(((PVX_SocketData*)Data)->SSL_Socket, data, (int)Size);
		}
		int TcpSocket::ReceiveAsync(void * data, size_t Size) {
			auto cr = CanReadAsync();
			if (cr > 0) {
				//if (Normal)
					return recv(((PVX_SocketData*)Data)->Socket, (char*)data, (int)Size, 0);
				//return SSL_read(((PVX_SocketData*)Data)->SSL_Socket, data, (int)Size);
			}
			return cr;
		}
		int TcpSocket::Receive(std::vector<unsigned char>& data) {
			int cnt;
			size_t sz = data.size();
			do {
				data.resize(sz + 1024);
				if (CanRead() < 0 || (cnt = recv(((PVX_SocketData*)Data)->Socket, ((char*)data.data()) + sz, 1024, 0)) < 0) {
					data.clear();
					return -1;
				}
				sz += cnt >= 0 ? cnt : 0;
			} while (cnt == 1024);
			data.resize(sz);
			return cnt;
		}

		int TcpSocket::ReceiveAsync(std::vector<unsigned char>& data) {
			int cnt;
			size_t sz = data.size();
			do {
				int sCount = CanReadAsync();
				if (sCount == 0) return 0;

				data.resize(sz + 1024);

				if (sCount < 0 || (cnt = recv(((PVX_SocketData*)Data)->Socket, ((char*)data.data()) + sz, 1024, 0)) < 0) {
					data.clear();
					return -1;
				}
				sz += cnt >= 0 ? cnt : 0;
			} while (cnt == 1024);
			data.resize(sz);
			return cnt;
		}

		const unsigned char * const TcpSocket::GetIP() {
			return (const unsigned char * const) (*(PVX_SocketData*)Data).Address.sa_data + 2;
		}

		int TcpSocket::CanRead() {
			fd_set sock{};
			FD_SET((*(PVX_SocketData*)Data).Socket, &sock);
			return select(0, &sock, 0, 0, 0);
		}
		int TcpSocket::CanReadAsync() {
			struct timeval to { 0, 1 };
			fd_set sock{};
			FD_SET((*(PVX_SocketData*)Data).Socket, &sock);
			return select(0, &sock, 0, 0, &to);
		}
		int TcpSocket::Receive(std::string & data) {
			int cnt;
			size_t sz = data.size();
			do {
				data.resize(sz + 1024);
				if (CanRead() < 0 || (cnt = recv(((PVX_SocketData*)Data)->Socket, ((char*)data.data()) + sz, 1024, 0)) < 0) {
					data.clear();
					return -1;
				}
				sz += cnt >= 0 ? cnt : 0;
			} while (cnt == 1024);
			data.resize(sz);
			return cnt;
		}
		int TcpSocket::SetReceiveTimeout(int ms) {
			struct timeval timeout;
			timeout.tv_sec = ms;
			timeout.tv_usec = ms;
			

			return setsockopt(((PVX_SocketData*)Data)->Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
			return setsockopt(((PVX_SocketData*)Data)->Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&ms, sizeof(int));
		}
		TcpSocket TcpSocket::Accept() {
			sockaddr info;
			int len = sizeof(sockaddr);
			CanRead();
			auto ret = accept(((PVX_SocketData*)Data)->Socket, &info, &len);
			//return{ ret, info };
			return{ (SOCKET_type)ret, &info };
		}

		void Network::TcpSocket::DeRef() {
			if(!--((PVX_SocketData*)Data)->Ref) {
				closesocket(((PVX_SocketData*)Data)->Socket);
				delete ((PVX_SocketData*)Data);
				Data = 0;
			}
			if(!--PVX_Socket_RefCount) WSACleanup();
		}
		TcpSocket::~TcpSocket() {
			DeRef();
		}
		int TcpSocket::Connect(const char * Url) {
			struct addrinfo hint = { 0 };
			struct addrinfo *dest;
			hint.ai_family = AF_UNSPEC;
			hint.ai_socktype = SOCK_DGRAM;

			int c = getaddrinfo(Url, 0, &hint, &dest);

			dest->ai_addr->sa_data[1] = 80;

			if(c)return 0;
			c = connect(((PVX_SocketData*)Data)->Socket, dest->ai_addr, (int)dest->ai_addrlen);
			freeaddrinfo(dest);
			return c;
		}
		int TcpSocket::Connect(const char * ip, const char * port) {
			struct addrinfo hint = { 0 };
			struct addrinfo *dest;
			hint.ai_family = AF_INET;
			hint.ai_protocol = IPPROTO_TCP;
			hint.ai_socktype = SOCK_STREAM;
			int c = getaddrinfo(ip, port, &hint, &dest);
			if(c)return c;
			c = connect(((PVX_SocketData*)Data)->Socket, dest->ai_addr, (int)dest->ai_addrlen);
			freeaddrinfo(dest);
			return c;
		}

		void TcpSocket::Disconnect() {
			if(Data) {
				closesocket(((PVX_SocketData*)Data)->Socket);
			}
		}
		int TcpSocket::IsOK() {
			return ((PVX_SocketData*)Data)->Socket != -1;
		}
		static std::mutex ThreadMutex;

		TcpServer::~TcpServer() {
			if(Running)Stop();
		}
		void TcpServer::Serve(std::function<void(TcpSocket)> Event) {
			Running = 1;
			ServingThread = new std::thread([this, Event]() {
				while(Running) {
					TcpSocket s = Accept();
					if(s.IsOK()) {
						{
							std::lock_guard<std::mutex> lock{ ThreadMutex };
							Sockets.insert(((PVX_SocketData*)(s.Data))->Socket);
							ThreadCount++;
						}
						Tasks.Enqueue([Event, s, this] {
							Event(s);
							{
								std::lock_guard<std::mutex> lock{ ThreadMutex };
								this->Sockets.erase(((PVX_SocketData*)(s.Data))->Socket);
								ThreadCount--;
							}
						});
					}
				}
			});
		}
		void TcpServer::Stop() {
			Running = 0;
			Disconnect();
			{
				std::lock_guard<std::mutex> lock{ ThreadMutex };
				for(auto s : Sockets) {
					closesocket(s);
				}
			}
			if(ServingThread && ServingThread->joinable())
				ServingThread->join();
			delete ServingThread;
			ServingThread = 0;
		}
	}
}
