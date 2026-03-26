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
	void StandartRelease(PVX::Network::socketData* dt);
};

namespace {
#ifndef __linux
	static WSAData Windows_Socket_Data;
#endif
	struct Callbacks {
		std::function<int(PVX::Network::socketData*, const void*, std::size_t)> Send;
		std::function<int(PVX::Network::socketData*, void*, std::size_t)> Recv;
		std::function<void(PVX::Network::socketData*)> Release;
	};
	Callbacks StandartActions{};


	SOCKET CreateListenSocket(const char* sPort) {
		struct addrinfo hint = { 0 };
		struct addrinfo* Source = 0;

		hint.ai_family = AF_INET;
		hint.ai_protocol = IPPROTO_TCP;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_flags = AI_PASSIVE;

		getaddrinfo(0, sPort, &hint, &Source);

		SOCKET SocketID = socket(Source->ai_family, Source->ai_socktype, Source->ai_protocol);
		if (SocketID == INVALID_SOCKET)
			return 0;

#ifdef __linux
		fcntl(SocketID, F_SETFL, fcntl(SocketID, F_GETFL, 0) | O_NONBLOCK);
#endif
		int enable = 1;
		setsockopt(SocketID, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));

		if (bind(SocketID, Source->ai_addr, (int)Source->ai_addrlen) == SOCKET_ERROR)
			return 0;

		freeaddrinfo(Source);
		listen(SocketID, SOMAXCONN);
		return SocketID;
	}

	//void socketData_Deleter(void* x) { delete (socketData*)x; }




}

namespace PVX::Network {

	struct socketData {
		socketData() = default;
		socketData(const socketData&) = default;
		socketData(SOCKET s, const struct sockaddr addr): Socket{ (SOCKET)s }, Address{ addr } {}
		~socketData() {
			Actions->Release(this);
		}

		SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
		Callbacks* Actions = &StandartActions;
		int Running = 1;
		struct sockaddr Address {};
		void* SSL = nullptr;
		int Send(const void* Data, size_t Size) { return Actions->Send(this, Data, Size); }
		int Recv(void* Data, size_t Size) { return Actions->Recv(this, Data, Size); }
	};


	void StandartRelease(PVX::Network::socketData* dt) {
		shutdown(dt->Socket, SD_BOTH);
		closesocket(dt->Socket);
	}

	class __initSockets {
	public:
		__initSockets() {
#ifndef __linux
			WSAStartup(MAKEWORD(2, 2), &Windows_Socket_Data);
#endif
			StandartActions.Send = [](socketData* sock, const void* Data, size_t Size) {
				size_t cur = 0;
				while (cur < Size) {
					int snd = send(sock->Socket, ((const char*)Data) + cur, (int)Size - cur, 0);
					cur += snd;
				}
				return (int)Size;
				//int snd = send(((socketData*)sock)->Socket, (const char*)Data, (int)Size, 0);
				//if (snd != Size) {
				//	return snd;
				//}
				//return snd;
			};
			StandartActions.Recv = [](socketData* sock, void* Data, size_t Size) {
				//if(Size>1){
				//	auto res = recv(((socketData*)sock)->Socket, (char*)Data, 1, 0);
				//	if(res!=1) 
				//		return res;
				//	return recv(((socketData*)sock)->Socket, ((char*)Data)+1, (int)Size-1, 0) + 1;
				//}
				return recv(sock->Socket, (char*)Data, (int)Size, 0);
			};
			StandartActions.Release = StandartRelease;
		}
#ifndef __linux
		~__initSockets() { WSACleanup(); }
#endif
	} ___initSockets;

	//TcpSocket::TcpSocket(): SocketData{ new socketData(), socketData_Deleter } {}
	TcpSocket::TcpSocket() : SocketData{ new socketData() } {}
	int TcpSocket::SetOption(TcpSocketOption Op, const void* Value, int ValueSize) {
		//return setsockopt(((socketData*)SocketData.get())->Socket, IPPROTO_TCP, (int)Op, (const char*)Value, ValueSize);
		return setsockopt(SocketData->Socket, IPPROTO_TCP, (int)Op, (const char*)Value, ValueSize);
	}
	int TcpSocket::SetOption(SocketOption Op, const void* Value, int ValueSize) {
		return setsockopt(SocketData->Socket, SOL_SOCKET, (int)Op, (const char*)Value, ValueSize);
	}
	TcpSocket::TcpSocket(uint32_t* socket, const void* addr): SocketData{ new socketData(SOCKET((size_t)socket), *(const sockaddr*)addr) } {}
	//TcpSocket::TcpSocket(uint32_t* socket, const void* addr): SocketData{ (void*)new socketData(SOCKET((size_t)socket), *(const sockaddr*)addr), socketData_Deleter } {}
	TcpSocket::TcpSocket(uint32_t* socket, const void* addr, std::function<void(void*)> deleter): SocketData{ 
		new socketData(SOCKET((size_t)socket), *(const sockaddr*)addr), 
		deleter 
	} {}

	int TcpSocket::CanRead() {
		fd_set sock{};
		//FD_SET(((socketData*)SocketData.get())->Socket, &sock);
		FD_SET(SocketData->Socket, &sock);
		return select(0, &sock, 0, 0, 0);
	}
	int TcpSocket::CanReadAsync() {
		struct timeval to { 0, 1 };
		fd_set sock{};
		FD_SET(SocketData->Socket, &sock);
		return select(0, &sock, 0, &sock, &to);
	}
	int TcpSocket::CanReadAsync(uint32_t ms) {
		struct timeval to { ms/1000, ms%1000 };
		fd_set sock{};
		FD_SET(SocketData->Socket, &sock);
		return select(0, &sock, 0, &sock, &to);
	}
	bool TcpSocket::IsConnected() {
		int error = 0;
		socklen_t len = sizeof(error);
		return !getsockopt(SocketData->Socket, SOL_SOCKET, SO_ERROR, (char*) & error, &len);
	}

	int TcpSocket::Send(const void* Data, size_t Size) {
		return SocketData->Actions->Send(SocketData.get(), Data, Size);
	}

	void TcpSocket::Disconnect() {
		SocketData->Actions->Release(this->SocketData.get());
	}

	size_t TcpSocket::SendFragmented(const std::vector<unsigned char>& data, size_t FragmentSize) {
		size_t Offset = 0;
		size_t sz = data.size();
		size_t SendSize = FragmentSize;
		while (Offset + FragmentSize < sz && SendSize == FragmentSize) {
			SendSize = Send((const void*)&data[Offset], FragmentSize);
			Offset += SendSize;
		}
		if (Offset < FragmentSize && SendSize >= 0) {
			SendSize = Send((const void*)&data[Offset], (int)(data.size() - Offset));
			Offset += SendSize;
		}
		return Offset;
	}

	int64_t TcpSocket::Receive(void* Data, size_t Size) {
		return int64_t(SocketData->Actions->Recv(SocketData.get(), Data, Size));
	}

	int64_t TcpSocket::ReceiveAsync(void* data, size_t Size) {
		auto cr = CanReadAsync();
		if (cr > 0) {
			return Receive(data, Size);
		}
		return cr;
	}
	int64_t TcpSocket::ReceiveAsync(std::vector<unsigned char>& data) {
		int64_t cnt;
		size_t sz = data.size();
		do {
			int sCount = CanReadAsync();
			if (sCount == 0)
				return 0;

			data.resize(sz + 1024);

			if (sCount < 0 || (cnt = Receive(data.data() + sz, 1024)) < 0) {
				data.clear();
				return -1;
			}
			sz += cnt;
		} while (cnt == 1024);
		data.resize(sz);
		return cnt;
	}

	std::vector<uint8_t> TcpSocket::Receive() {
		std::vector<uint8_t> ret(1024);
		int rsz;
		size_t cur = 0;
		for (;;) {
			//if (CanRead() < 0 || (rsz = Receive(ret.data() + cur, 1024))<0) {
			//	return {};
			//}
			if ((rsz = Receive(ret.data() + cur, 1024))<0) {
				return {};
			}
			if (rsz < 1024) {
				ret.resize(cur + rsz);
				return ret;
			}
			cur += 1024;
			ret.resize(cur + 1024);
		}
	}
	//int64_t TcpSocket::Receive(std::vector<uint8_t>& Data) {
	//	int64_t rsz;
	//	size_t cur = Data.size();
	//	Data.resize(cur + 1024);
	//	for (;;) {
	//		//if (CanRead() < 0 || (rsz = Receive(Data.data() + cur, 1024))<0) {
	//		//	Data.clear();
	//		//	return -1;
	//		//}
	//		//if (CanReadAsync() < 0 || (rsz = Receive(Data.data() + cur, 1024))<0) {
	//		//	Data.clear();
	//		//	return -1;
	//		//}
	//		if ((rsz = Receive(Data.data() + cur, 1024))<0) {
	//			Data.clear();
	//			return -1;
	//		}
	//		if (rsz < 1024) {
	//			Data.resize(cur + rsz);
	//			return int64_t(Data.size());
	//		}
	//		cur += 1024;
	//		Data.resize(cur + 1024);
	//	}
	//}
	int64_t TcpSocket::Receive(std::vector<uint8_t>& Data) {
		int64_t rsz;
		size_t cur = Data.size();
		Data.resize(cur + 1024);


		//int testConnection;
		//while(!(testConnection = CanReadAsync()));
		//if(testConnection < 0) {
		//	Data.clear();
		//	return -1;
		//}

		//if(CanReadAsync(100)<=0) {
		//	Data.clear();
		//	return -1;
		//}

		for(;;) {
			if((rsz = Receive(Data.data() + cur, 1024)) < 0) {
				Data.clear();
				return -1;
			}
			if(rsz < 1024) {
				Data.resize(cur + rsz);
				return int64_t(Data.size());
			}
			cur += 1024;
			Data.resize(cur + 1024);
		}
	}
	int64_t TcpSocket::Receive(std::string& Data) {
		int64_t rsz;
		size_t cur = Data.size();
		Data.resize(cur + 1024);
		for (;;) {
			//if (CanRead() < 0 || (rsz = Receive(Data.data() + cur, 1024))<0) {
			//	Data.clear();
			//	return -1;
			//}
			if ((rsz = Receive(Data.data() + cur, 1024))<0) {
				Data.clear();
				return -1;
			}
			if (rsz < 1024) {
				Data.resize(cur + rsz);
				return int64_t(Data.size());
			}
			cur += 1024;
			Data.resize(cur + 1024);
		}
	}
	int TcpSocket::Connect(const char* Url, const char* Port) {
		struct addrinfo hint = { 0 };
		struct addrinfo* dest;
		hint.ai_family = AF_INET;
		hint.ai_protocol = IPPROTO_TCP;
		hint.ai_socktype = SOCK_STREAM;
		int c = getaddrinfo(Url, Port, &hint, &dest);
		if (c)return c;
		c = connect(SocketData->Socket, dest->ai_addr, (int)dest->ai_addrlen);
		freeaddrinfo(dest);
		return c;
	}

	void TcpServer::init(int ThreadCount) {
		Running = ServingSocket != nullptr;
		if (ThreadCount <= 0) ThreadCount = std::max(1, (int)std::thread::hardware_concurrency() - 1);
		for (auto i = 0; i < ThreadCount; i++) {
			Workers.push_back(std::thread([this] {
				for (;;) {
					std::function<void()> NextTask;
					{
						std::unique_lock<std::mutex> lock{ TaskMutex };
						//	TaskAdded.wait_for(lock, std::chrono::seconds(1), [this] { return !Tasks.empty() || !Running; });
						TaskAdded.wait(lock, [this] { return !Tasks.empty() || !Running; });
						if (!Running) return;
						if (!Tasks.empty()) {
							NextTask = std::move(Tasks.front());
							Tasks.pop();
						}
						else continue;
					}
					Working++;
					NextTask();
					Working--;
				}
			}));
		}
	}

	TcpServer::TcpServer(const char* Port, int ThreadCount) : ServingSocket{ (uint32_t*)(size_t)CreateListenSocket(Port) } {
		init(ThreadCount);
	}
	TcpServer::TcpServer(std::function<void(TcpSocket)> clb, const char* Port, int ThreadCount) {
		init(ThreadCount);
		Serve(clb);
	}
	TcpServer::TcpServer(std::function<void(TcpSocket)> clb, std::function<void(TcpSocket&)> OnConnect, const char* Port, int ThreadCount) {
		init(ThreadCount);
		Serve(clb, OnConnect);
	}

	TcpServer::~TcpServer() {
		Running = false;
		closesocket((SOCKET)(size_t)ServingSocket);
		Stop();
	}

#ifdef __linux
	SOCKET Accept(SOCKET s, struct sockaddr * addr, socklen_t* addrlen) {
		fd_set set;
		struct timeval timeout;
		sockaddr info;
		socklen_t len = sizeof(sockaddr);
		int rv = 0;
		int to = 0;
		while (rv == 0) {
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			FD_ZERO(&set);
			FD_SET((SOCKET)(size_t)s, &set);
			rv = select(((SOCKET)(size_t)s) + 1, &set, NULL, NULL, &timeout);
		}
		if (rv==-1) return -1;
		return accept((SOCKET)(size_t)s, &info, &len);
	}
#else
	inline SOCKET Accept(SOCKET s, sockaddr* addr, socklen_t* addrlen) {
		return accept(s, addr, addrlen);
	}
#endif

	int debugSocketCount = 0;

	void TcpServer::Serve(std::function<void(TcpSocket)> Event, std::function<void(TcpSocket&)> Transform) {
		if ((Running = ServingSocket != nullptr)) {
			ServingThread = std::make_unique<std::thread>([this, Event, Transform] {
				while (Running) {
					sockaddr info;
					socklen_t len = sizeof(sockaddr);
					auto ss = Accept((SOCKET)(size_t)ServingSocket, &info, &len);
					if (ss != -1) {
						TcpSocket mySocket = TcpSocket{ (uint32_t*)(size_t)ss, (void*)&info };
						if (Transform) Transform(mySocket);
						{
							std::lock_guard<std::mutex> slock{ SocketGuard };
							OpenSockets[(uint32_t*)(size_t)ss] = mySocket;
							debugSocketCount++;
						}
						{
							std::lock_guard<std::mutex> lock{ TaskMutex };
							Tasks.push([mySocket, ss, Event, this] {
								auto MySocket = mySocket;
								if (MySocket.CanRead() != SOCKET_ERROR)
									Event(MySocket);
								{
									std::lock_guard<std::mutex> slock{ SocketGuard };
									OpenSockets.erase((uint32_t*)(size_t)ss);
								}
							});
						}
						TaskAdded.notify_one();
					}
				}
				for (auto& t : Workers) t.join();
			});
		}
	}
	void TcpServer::Stop() {
		Running = false;
		{
			std::lock_guard<std::mutex> slock{ SocketGuard };
			for (auto& s : OpenSockets) {
				s.second.Disconnect();
			}
		}
		TaskAdded.notify_all();
		if (ServingThread.get() != nullptr && ServingThread->joinable()) ServingThread->join();
	}
}