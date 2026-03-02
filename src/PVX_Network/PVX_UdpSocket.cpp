#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <functional>

#ifndef __linux
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "fwpuclnt.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
using SOCKET = int;
#define closesocket close
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define SD_BOTH SHUT_RDWR
#endif

#include <PVX_Network.h>


namespace PVX::Network {
	struct udpData;

	void socketData_Deleter(void* x) { delete (udpData*)x; }

	struct udpData {
		SOCKET Socket = {};
		addrinfo * Listen = {};
		SOCKADDR_IN From = {};
		SOCKADDR_IN To = {};

		udpData(int Port) {
			Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (!Port) return;

			char port[6]{ 0,0,0,0,0,0 };
			int c = 4;


			while (Port) {
				port[c--] = (Port % 10) + '0';
				Port /= 10;
			}
			c++;

			addrinfo hint = { 0 };

			hint.ai_family = AF_INET;
			hint.ai_protocol = IPPROTO_UDP;
			hint.ai_socktype = SOCK_STREAM;
			hint.ai_flags = AI_PASSIVE;

			getaddrinfo(NULL, port + c, &hint, &Listen);
			if (bind(Socket, Listen->ai_addr, Listen->ai_addrlen) == SOCKET_ERROR) {
				int err = 1;
				return;
			}
		}
		void Release() {
			if (Listen) freeaddrinfo(Listen);
			Listen = {};
			if (Socket) {
				shutdown(Socket, SD_BOTH);
				closesocket(Socket);
			}
			Socket = {};
		}
		~udpData() {
			Release();
		}
		int Receive(const void * data, int Size) {
			int sz = sizeof(SOCKADDR_IN);
			return recvfrom(Socket, (char*)data, Size, 0, (SOCKADDR*)&From, &sz);
		}
		int Receive(std::vector<uint8_t>& ret) {
			int sz, retSize = ret.size();
			ret.resize(retSize + 1024);

			for (;;) {
				sz = Receive(&ret[retSize], 1024);
				if (sz < 0) 
					return sz;
				retSize += sz;
				if (sz != 1024) break;
				ret.resize(retSize + 1024);
			}
			ret.resize(retSize);
			return retSize;
		}
		int Receive(std::string & ret) {
			int sz, retSize = ret.size();
			ret.resize(retSize + 1024);

			for (;;) {
				sz = Receive(&ret[retSize], 1024);
				if (sz < 0) 
					return sz;
				retSize += sz;
				if (sz != 1024) break;
				ret.resize(retSize + 1024);
			}
			ret.resize(retSize);
			return retSize;
		}

		IN_ADDR WhoAmI() {
			SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
			if (connect(s, (const sockaddr*)&From, sizeof(From)) == SOCKET_ERROR) {
				closesocket(s);
				return {};
			}
			sockaddr_in localAddr{};
			int addrLen = sizeof(localAddr);

			if (getsockname(s, (sockaddr*)&localAddr, &addrLen) == SOCKET_ERROR) {
				closesocket(s);
				return {};
			}

			closesocket(s);
			return localAddr.sin_addr;
		}

		int Send(const void * data, int Size) {
			return sendto(Socket, (char*)data, Size, 0, (SOCKADDR*)&To, sizeof(sockaddr_in));
		}
		int Reply(const void * data, int Size) {
			return sendto(Socket, (char*)data, Size, 0, (SOCKADDR*)&From, sizeof(sockaddr_in));
		}
		void SetDestination(const char * Url, unsigned short Port) {
			To.sin_family = AF_INET;
			To.sin_port = htons(Port);
			inet_pton(AF_INET, Url, &To.sin_addr.s_addr);
			//To.sin_addr.s_addr = inet_addr(Url);
		}
		int SetOption(int Level, int optname, const char * optval, int optlen) {
			return setsockopt(Socket, Level, optname, optval, optlen);
		}
		void AddMembership(const char * url, uint32_t port) {
			ip_mreq mreq{};
			inet_pton(AF_INET, url, &mreq.imr_multiaddr);
			mreq.imr_interface.s_addr = htonl(port);

			SetOption(IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
		}
	};



	UdpSocket::UdpSocket(int Port): SocketData{ (void*)new udpData(Port), socketData_Deleter } {}
	int UdpSocket::Receive(const void * data, int Size) {
		return GetInternalData<udpData>().Receive(data, Size);
	}
	int UdpSocket::Receive(std::vector<uint8_t>& ret) {
		return GetInternalData<udpData>().Receive(ret);
	}
	int UdpSocket::Receive(std::string & ret) {
		return GetInternalData<udpData>().Receive(ret);
	}
	int UdpSocket::Send(const void * data, int Size) {
		return GetInternalData<udpData>().Send(data, Size);
	}
	int UdpSocket::Reply(const void * data, int Size) {
		return GetInternalData<udpData>().Reply(data, Size);
	}
	void UdpSocket::SetDestination(const char * Url, unsigned short Port) {
		GetInternalData<udpData>().SetDestination(Url, Port);
	}
	void UdpSocket::Release() {
		GetInternalData<udpData>().Release();
	}
	void UdpSocket::mDNS() {
		auto& dt = GetInternalData<udpData>();
		BOOL reuse = TRUE;
		int ttl = 255;
		BOOL loop = FALSE;

		dt.SetOption(SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
		dt.SetOption(IPPROTO_IP, IP_TTL, (char*)&ttl, sizeof(ttl));
		//dt.SetOption(IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loop, sizeof(loop));

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(5353);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		bind(dt.Socket, (sockaddr*)&addr, sizeof(addr));

		dt.AddMembership("224.0.0.251", INADDR_ANY);
	}

	IN_ADDR UdpSocket::getSenderIP() {
		return GetInternalData<udpData>().From.sin_addr;
	}
	uint16_t UdpSocket::getSenderPort() {
		return GetInternalData<udpData>().From.sin_port;
	}
	IN_ADDR UdpSocket::getIP() {
		return GetInternalData<udpData>().To.sin_addr;
	}
	uint16_t UdpSocket::getPort() {
		return GetInternalData<udpData>().To.sin_port;
	}

	struct mdnsHeader {
		uint16_t TransactionID;
		uint16_t Flags;
		uint16_t QDCOUNT;
		uint16_t ANCOUNT;
		uint16_t NSCOUNT;
		uint16_t ARCOUNT;
		uint8_t Data[1];
	};
	struct mdnsResponce {
		uint16_t Type;
		uint16_t Class;
		uint32_t TTL;
		uint16_t rdLength;
		uint8_t Ip[4];
	};

	mDNS::mDNS(const std::string& domain): Running{ true }, Socket { 0 }, mDNS_thread {
		[this, domain] {
			std::vector<uint8_t> response(12 + 1 + domain.size() + 1 + 5 + 1 + 2 + 2 + 4 + 2 + 4);
			bool noIp = true;
			
			auto& h1 = *(mdnsHeader*)response.data();
			h1.TransactionID = 0;
			h1.Flags = 0x0084;
			h1.QDCOUNT = 0;
			h1.ANCOUNT = 0x0100;
			h1.NSCOUNT = 0;
			h1.ARCOUNT = 0;
			h1.Data[0] = domain.size();
			memcpy(h1.Data + 1, domain.data(), domain.size());
			h1.Data[domain.size() + 1] = 5;
			memcpy(h1.Data + 2 + domain.size(), "local\0", 6);
			auto& tail = *(mdnsResponce*)(h1.Data + 2 + domain.size() + 6);
			tail.Type = 0x0100;
			tail.Class = 0x0180;
			tail.TTL = 0x78000000;
			tail.rdLength = 0x0400;
			

			Socket.mDNS();
			Socket.SetDestination("224.0.0.251", 5353);
			auto myIP = Socket.getIP();
			std::vector<uint8_t> Data;
			std::string in_sub_domain, in_domain;
			in_sub_domain.resize(domain.size());
			in_domain.resize(5);
			while (Running) {
				Data.clear();
				Socket.Receive(Data);

				auto& header = *(mdnsHeader*)Data.data();
				if (header.QDCOUNT == 0x0100 && header.Data[0] == domain.size() && header.Data[domain.size()+1]==5) {
					memcpy(in_sub_domain.data(), header.Data + 1, header.Data[0]);
					memcpy(in_domain.data(), header.Data + 2 + domain.size(), 5);
					if (in_sub_domain == domain && in_domain == "local") {
						if (noIp) {
							auto ip = Socket.GetInternalData<udpData>().WhoAmI();
							memcpy(tail.Ip, &ip.S_un.S_un_b, 4);

							Socket.GetInternalData<udpData>().SetOption(IPPROTO_IP, IP_MULTICAST_IF, (char*)&ip.S_un.S_un_b, sizeof(ip));
							noIp = false;
						}

						auto otherIp = Socket.getSenderIP();
						printf("responding to %d.%d.%d.%d\n", 
							otherIp.S_un.S_un_b.s_b1, 
							otherIp.S_un.S_un_b.s_b2, 
							otherIp.S_un.S_un_b.s_b3, 
							otherIp.S_un.S_un_b.s_b4);

						auto& tail2 = *(mdnsResponce*)(header.Data + 2 + domain.size() + 6);
						
						h1.TransactionID = header.TransactionID;
						h1.Flags = 0x0084 | header.Flags;

						//Socket.Reply(response.data(), response.size());
						Socket.Send(response.data(), response.size());
					}
				}
			}
		}
	} {}
	mDNS::~mDNS() {
		Running = false;
		Socket.Release();
		mDNS_thread.join();
	}
}
