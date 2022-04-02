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
using SOCKET = int;
#define closesocket close
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif

#include <PVX_Network.h>

namespace {
	using SendCLB = int (*)(void* dt, const void*, size_t sz);
	using ReceiveCLB = int (*)(void* sock, void*, size_t sz);
	using ReleaseCLB = void (*)(void*);

#ifdef _WINDOWS
	static WSAData Windows_Socket_Data;
#endif
	struct Callbacks {
		SendCLB Send;
		ReceiveCLB Recv;
		ReleaseCLB Release;
	};
	Callbacks StandartActions{};

	void StandartRelease(void* dt) {
		closesocket(*(SOCKET*)dt);
	}

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
		if (bind(SocketID, Source->ai_addr, (int)Source->ai_addrlen) == SOCKET_ERROR)
			return 0;

		freeaddrinfo(Source);
		listen(SocketID, SOMAXCONN);
		return SocketID;
	}


	class socketData {
	public:
		socketData() = default;
		socketData(uint32_t* s, const struct sockaddr addr) : Socket{ (SOCKET)s }, Address{ addr } {}
		~socketData() { Actions->Release(this); }

		SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
		Callbacks* Actions = &StandartActions;
		int Running = 1;
		struct sockaddr Address {};
		void* SSL = nullptr;
		int Send(const void* Data, size_t Size) { return Actions->Send(this, Data, Size); }
		int Recv(void* Data, size_t Size) { return Actions->Recv(this, Data, Size); }
	};

	void socketData_Deleter(void* x) { delete (socketData*)x; }

	class __initSockets {
	public:
		__initSockets() {
#ifndef __linux
			WSAStartup(MAKEWORD(2, 2), &Windows_Socket_Data);
#endif
			StandartActions.Send = [](void* sock, const void* Data, size_t Size) {
				return send(((socketData*)sock)->Socket, (const char*)Data, (int)Size, 0);
			};
			StandartActions.Recv = [](void* sock, void* Data, size_t Size) {
				return recv(((socketData*)sock)->Socket, (char*)Data, (int)Size, 0);
			};
			StandartActions.Release = StandartRelease;
		}
#ifndef __linux
		~__initSockets() { WSACleanup(); }
#endif
	} ___initSockets;


}

namespace PVX::Network {
	TcpSocket::TcpSocket() : SocketData{ (void*)new socketData(), socketData_Deleter } {}
	int TcpSocket::SetOption(TcpSocketOption Op, const void* Value, int ValueSize) {
		return setsockopt(((socketData*)SocketData.get())->Socket, IPPROTO_TCP, (int)Op, (const char*)Value, ValueSize);
	}
	int TcpSocket::SetOption(SocketOption Op, const void* Value, int ValueSize) {
		return setsockopt(((socketData*)SocketData.get())->Socket, SOL_SOCKET, (int)Op, (const char*)Value, ValueSize);
	}
	TcpSocket::TcpSocket(uint32_t* socket, const void* addr) : SocketData{ (void*)new socketData(socket, *(const sockaddr*)addr), socketData_Deleter } {}

	int TcpSocket::CanRead() {
		fd_set sock{};
		FD_SET(((socketData*)SocketData.get())->Socket, &sock);
		return select(0, &sock, 0, 0, 0);
	}
	int TcpSocket::CanReadAsync() {
		struct timeval to { 0, 1 };
		fd_set sock{};
		FD_SET(((socketData*)SocketData.get())->Socket, &sock);
		return select(0, &sock, 0, 0, &to);
	}

	int TcpSocket::Send(const void* Data, size_t Size) {
		return ((socketData*)SocketData.get())->Actions->Send(SocketData.get(), Data, Size);
	}

	void TcpSocket::Disconnect() {
		((socketData*)SocketData.get())->Actions->Release(this);
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

	int TcpSocket::Receive(void* Data, size_t Size) {
		return ((socketData*)SocketData.get())->Actions->Recv(SocketData.get(), Data, Size);
	}

	int TcpSocket::ReceiveAsync(void* data, size_t Size) {
		auto cr = CanReadAsync();
		if (cr > 0) {
			return Receive(data, Size);
		}
		return cr;
	}
	int TcpSocket::ReceiveAsync(std::vector<unsigned char>& data) {
		int cnt;
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
			if ( (rsz = Receive(ret.data() + cur, 1024))<0) {
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
	int TcpSocket::Receive(std::vector<uint8_t>& Data) {
		int rsz;
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
				return Data.size();
			}
			cur += 1024;
			Data.resize(cur + 1024);
		}
	}
	int TcpSocket::Receive(std::string& Data) {
		int rsz;
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
				return Data.size();
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
		c = connect(((socketData*)SocketData.get())->Socket, dest->ai_addr, (int)dest->ai_addrlen);
		freeaddrinfo(dest);
		return c;
	}

	TcpServer::TcpServer(const char* Port, int ThreadCount) :
		ServingSocket{ (uint32_t*)CreateListenSocket(Port) } {
		Running = true;
		if (ThreadCount<=0)ThreadCount = std::max(1, (int)std::thread::hardware_concurrency() - 1);
		for (auto i = 0; i<ThreadCount; i++) {
			Workers.push_back(std::thread([this] {
				for (;;) {
					std::function<void()> NextTask;
					{
						std::unique_lock<std::mutex> lock{ TaskMutex };
						TaskAdded.wait(lock, [this] { return Tasks.size() || !Running; });
						if (Running) {
							NextTask = std::move(Tasks.front());
							Tasks.pop();
						} else
							return;
					}
					NextTask();
					TaskEnded.notify_one();
				}
			}));
		}
	}
	TcpServer::~TcpServer() {
		closesocket((SOCKET)ServingSocket);
		Stop();
	}
	void TcpServer::Serve(std::function<void(TcpSocket)> Event, std::function<void(TcpSocket&)> Transform) {
		Running = true;
		ServingThread = std::make_unique<std::thread>([this, Event, Transform] {
			while (Running) {
				sockaddr info;
				socklen_t len = sizeof(sockaddr);
				auto ss = accept((SOCKET)ServingSocket, &info, &len);
				if (ss != -1) {
					{
						std::lock_guard<std::mutex> slock{ SocketGuard };
						OpenSockets.insert((uint32_t*)ss);
					}
					std::lock_guard<std::mutex> lock{ TaskMutex };
					{
						Tasks.push([ss, info, Event, Transform, this] {
							TcpSocket mySocket{ (uint32_t*)ss, (void*)&info };
							if (Transform) Transform(mySocket);
							Event(mySocket);
							{
								std::lock_guard<std::mutex> slock{ SocketGuard };
								OpenSockets.erase((uint32_t*)ss);
							}
						});
					}
					TaskAdded.notify_one();
				}
			}
			for (auto& t : Workers) t.join();
		});
	}
	void TcpServer::Stop() {
		Running = false;
		for (auto s : OpenSockets) closesocket((SOCKET)s);
		TaskAdded.notify_all();
		if (ServingThread.get() != nullptr) ServingThread->join();
	}
}