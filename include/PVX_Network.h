#ifndef __PVX_NETWORK_H__
#define __PVX_NETWORK_H__

//#define WIN32_LEAN_AND_MEAN
//
//#include <winsock2.h>
//#include <ws2tcpip.h>
#include <functional>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <set>
#include <regex>
#include <map>
#include <PVX_DataBuilder.h>
#include <PVX_Threading.h>
#include <shared_mutex>
#include <atomic>

typedef unsigned long long SOCKET_type;

// C:\Users\PerVertEX\Google Drive\C++ Projects 2016\Libraries64\OpenSSL\bin64

namespace PVX {
	namespace Network {
		class HttpResponse;
		class WebSocket;
		class HttpRequest;
		class HttpServer;
		class WebSocketServer;
		class WebSocketPacket;

		//class OpenSSL {
		//public:
		//	OpenSSL();
		//	~OpenSSL();
		//	void SetCertificate(const char * Certificate);
		//	void SetCertificate(const char * Certificate, const char * Pass);
		//	OpenSSL(const char * Certificate);
		//	OpenSSL(const char * Certificate, const char * Pass);
		//private:
		//	void * Data;
		//	friend class TcpSocket;
		//};
		class TcpSocket {
		public:
			TcpSocket();
			TcpSocket(const TcpSocket&);
			TcpSocket(const char * Port);
			~TcpSocket();
			TcpSocket & operator=(const TcpSocket & s);
			int Send(const void * data, size_t sz);
			int Send(const std::vector<unsigned char> & Data);
			int Send(const std::string & Data);

			size_t SendFragmented(const std::vector<unsigned char> & Data, size_t FragmentSize = 0x00800000);

			int Receive(void * Data, size_t Size);
			int Receive(std::vector<unsigned char> & Data);
			int Receive(std::string & Data);
			int ReceiveAsync(std::vector<unsigned char> & Data);
			int ReceiveAsync(void * Data, size_t Size);

			int SetReceiveTimeout(int ms);

			TcpSocket Accept();
			int Connect(const char * Url);
			int Connect(const char * ip, const char * port);
			//void SetSSL(const OpenSSL&);
			void Disconnect();
			int IsOK();
			int CanRead();
			int CanReadAsync();
			const unsigned char * const GetIP();
			template<typename T> 
			inline T & GetPrivateData() {
				return *(T*)PrivateData;
			}
			std::string RemoteAddress() const;
			std::wstring wRemoteAddress() const;
			// int SecureConnection();
		protected:
			TcpSocket(const SOCKET_type, const void*);
			//TcpSocket(const SOCKET, const sockaddr &);
			void * Data, * PrivateData;
			void DeRef();
			friend class TcpServer_Old;
			friend class TcpServer;
		};
		
		void SecureSocket();

		class TcpServer: public TcpSocket {
		public:
			TcpServer(const char* Post) : TcpSocket(Post), ThreadCount{ 0 }, ServingThread{ nullptr } {};
			~TcpServer();
			void Serve(std::function<void(TcpSocket)> Event);
			void Stop();
		protected:
			std::atomic_int Running, ThreadCount;
			std::thread * ServingThread;
			PVX::Threading::TaskPump Tasks;
			std::set<SOCKET_type> Sockets;
		};

		class UtfHelper {
			std::wstring Text;
		public:
			UtfHelper() {};
			UtfHelper(const std::wstring &);
			UtfHelper(const wchar_t *);
			UtfHelper(const std::vector<unsigned char> &);
			UtfHelper(const std::string &);
			UtfHelper & operator=(const std::wstring &);
			std::wstring * operator->();
			std::wstring & operator()();
			operator std::wstring&();
			operator std::wstring() const;
		};

		typedef struct SimpleTuple {
			std::wstring Name, Value;
		} SimpleTuple;

		struct MultipartFormPartItem {
			std::wstring Value;
			std::map<std::wstring, std::wstring> Values;
		};

		struct MultipartFromPart {
			std::map<std::wstring, MultipartFormPartItem> Options;
			size_t Start, Size;
		};

		class HttpRequest {
		public:
			class RequestRange {
			public:
				size_t Start, End;
			};
			std::string Method;
			std::string Protocol;
			std::string Version;
			//UtfHelper QueryString;
			std::wstring QueryString;
			UtfHelper Get;
			std::map<std::string, UtfHelper> Headers;
			HttpRequest();
			HttpRequest(const std::string & s);
			HttpRequest & operator=(const std::string & s);
			std::vector<unsigned char> RawContent;
			std::wstring * HasHeader(const std::string & h);
			PVX::JSON::Item Json() const;
			WebSocket GetWebSocket();
			bool SouldCompress();
			std::vector<RequestRange> GetRanges();
			std::string RawHeader;
			std::map<std::wstring, std::wstring> Cookies;
			std::wstring SessionId;
			std::vector<MultipartFromPart> Multipart;
			std::vector<unsigned char> GetMultiFormData(const MultipartFromPart& part);
			std::wstring operator[](const std::wstring& Name);
			long long operator()(const std::wstring & Name);
			TcpSocket Socket;
			std::map<std::wstring, std::wstring> GetVariableMap() const;
			void BasicAuthentication(std::function<void(const std::wstring& Username, const std::wstring& Password)> clb);

			PVX::JSON::Item User;
		private:
			std::map<std::wstring, UtfHelper> Variables;
			void SetMultipartForm(const std::wstring & bound);
			HttpServer * Server;
			int SetHeader(const std::string & s);
			friend class HttpServer;
		};

		class HttpResponse {
		public:
			HttpResponse();
			UtfHelper & operator[](const std::wstring &);
			int StatusCode;
			PVX_DataBuilder Content;
			void Json(const PVX::JSON::Item & json);
			void Redirect(const std::wstring & Location, int Status = 303);
			void Json(const std::wstring & json);
			void Json(const std::vector<unsigned char> & json);
			void Html(const std::wstring & html);
			void Html(const std::string & html);
			void UtfData(const std::wstring & data, const std::wstring & contentType);

			template<typename T>
			void Data(const std::vector<T>& data, const std::wstring& contentType) {
				Content.SetData(data.data(), data.size() * sizeof(T));
				Headers[L"content-type"] = contentType;
			}

			void BeginRange(size_t Size);
			void AddRange(const std::wstring & ContentType, size_t Offset, const std::vector<unsigned char> & Data);
			void EndRange();
			int StreamFile(const std::wstring& Filename, int BufferSize = 0xffff, const std::wstring & Mime = L"");
			int StreamFile(HttpRequest& Request, const std::wstring& Filename, int BufferSize = 0xffff);
			int ServeFile(const std::wstring & Filename, const std::wstring & Mime = L"");

			int SingleRangeFile(size_t Offset, size_t Size, const std::wstring & Filename, int FragmentSize=0x00100000);

			// return true if More data sould me sent
			void StreamRaw(size_t Size, std::function<bool(TcpSocket & Socket)> fnc);

			bool SouldCompress, Handled;
			void SetCookie(const std::wstring & Name, const std::wstring & Value);
			void ClearCookie(const std::wstring & Name);
			int SendHeader(size_t ContentLength = 0);
			void AllowOrigin(HttpRequest& req, const std::set<std::wstring>& Allow = {});

			void MakeWebToken(const PVX::JSON::Item& User);
		protected:
			TcpSocket Socket;
			size_t RangeSize;

			HttpServer * Server;
			typedef struct { 
				size_t Size;
				std::function<bool(TcpSocket&)> Func;
			} myStream;
			std::vector<myStream> Streams;
			std::map<std::wstring, UtfHelper> Headers;
			std::vector<SimpleTuple> MoreHeaders;
			friend class HttpServer;
			friend class Controller;
		};

		class Route {
		public:
			Route(const std::wstring & url, std::function<void(HttpRequest&, HttpResponse&)> Action);
			int Match(const std::wstring & url, std::map<std::wstring, UtfHelper> & Vars, UtfHelper & Query);
			int Run(HttpRequest &, HttpResponse &);
			void ResetAction(std::function<void(HttpRequest&, HttpResponse&)> Action);
		private:
			std::wstring OriginalRoute;
			std::vector<std::wstring> Names;
			std::function<void(HttpRequest&, HttpResponse&)> Action;
			std::wregex Matcher;
		};

		typedef struct HttpContext {
			HttpRequest * Request;
			HttpResponse * Response;
			HttpServer * Server;
			int StatusCode;
			std::map<std::wstring, std::wstring> Params;
		} HttpContext;

		class HttpServer {
			friend class HttpResponse;
		public:
			HttpServer();
			HttpServer(const std::wstring & ConfigFile);
			~HttpServer();

			std::function<void(TcpSocket)> GetHandler();

			void Routes(const Route & routes);
			void Routes(const std::wstring & url, std::function<void(HttpRequest&, HttpResponse&)> Action);
			void Routes(const std::initializer_list<Route> & routes);

			void AddFilter(std::function<int(HttpRequest&, HttpResponse&)> Filter);

			void SetDefaultRoute(std::function<void(HttpRequest&, HttpResponse&)> Action);
			void Start();
			void Stop();
			WebSocketServer & CreateWebSocketServer(const std::wstring& Url);
			WebSocketServer & CreateWebSocketServer(const std::wstring & Url, std::function<void(WebSocketServer&, const std::string&, const std::vector<unsigned char>&)> clb);
			const std::wstring & GetMime(const std::wstring & extension) const;

			// Route must contain {Path} Variable
			std::function<void(HttpRequest&, HttpResponse&)> ContentServer(const std::wstring & ContentPath = L"");
			Route ContentPathRoute(const std::wstring & url, const std::wstring & Path);
			void DefaultRouteForContent(const std::wstring & Path);
			std::wstring StartSession(PVX::Network::HttpRequest & req, PVX::Network::HttpResponse & resp);

			void BasicAuthentication(std::function<void(const std::wstring&, const std::wstring&)> clb);
			void EnableWebToken(const std::string& Key);
		protected:
			int SendResponse(TcpSocket&, HttpRequest & http, HttpResponse&);
			void HandleRequest(TcpSocket &, HttpRequest &, Route & );
			void SetDefaultHeader(HttpResponse &);
			std::wstring MakeSession();

			int HandleWebToken(HttpRequest& req, HttpResponse& resp);

			std::vector<std::unique_ptr<WebSocketServer>> WebSocketServers;
			std::vector<Route> Router;
			Route DefaultRoute;
			std::map<std::wstring, std::wstring> Mime;
			std::vector<SimpleTuple> DefaultHeader;
			std::vector<std::function<int(HttpRequest&, HttpResponse&)>> Filters;
			std::set<std::wstring> Sessions;
			std::string TokenKey;
			friend class WebSocket;
		};

		class WebSocketPacket {
		protected:
			std::vector<unsigned char> Content;
			int Opcode;
			friend class WebSocket;
			friend class WebSocketServer;
		public:
			void Text(const std::string & Text);
			void Text(const std::wstring & Text);
			void Json(const PVX::JSON::Item & Json);
			void Run(const std::wstring & Function, const PVX::JSON::Item & Params);
			void Data(const void * data, size_t Size);
			void Data(const std::vector<unsigned char> & data);
		};

		class WebSocket {
		protected:
			WebSocket();
			TcpSocket Socket;
			HttpServer * Server;
			friend class HttpRequest;
			friend class HttpServer;
			friend class WebSocketServer;
			friend class WebSocket;
		public:
			enum class WebSocker_Opcode {
				Opcode_Continued = 0,
				Opcode_Text,
				Opcode_Bynary,
				Opcode_ReservedNonControl0,
				Opcode_ReservedNonControl1,
				Opcode_ReservedNonControl2,
				Opcode_ReservedNonControl3,
				Opcode_ReservedNonControl4,
				Opcode_Close,
				Opcode_Ping,
				Opcode_Pong,
				Opcode_ReservedControl0,
				Opcode_ReservedControl1,
				Opcode_ReservedControl2,
				Opcode_ReservedControl3,
				Opcode_ReservedControl4
			};
			bool CanRead();
			int Receive(std::vector<unsigned char> & Data);
			int Receive();
			WebSocket(const TcpSocket & sock);
			int operator()(std::function<void(WebSocketPacket&)> Event);
			std::vector<unsigned char> Message;
			WebSocker_Opcode Opcode;
			int HasMore;
			void Disconnect();
		};

		class WebSocketServer {
		protected:
			friend class HttpServer;
			std::shared_mutex ConnectionMutex;
			std::function<void(const std::string&, WebSocket&)> onConnect;
			std::function<void(const std::string&)> onDisconnect;
			std::map<std::string, std::function<void(WebSocketPacket&)>> ServerActions;
			std::map<std::string, std::function<void(PVX::JSON::Item&, const std::string&)>> ClientActions;
			std::map<std::string, std::function<void(const std::vector<unsigned char>&, const std::string&)>> ClientActionsRaw;
			std::map<std::string, std::set<std::string>> GroupConnections, ConnectionGroups;
			std::function<void(HttpRequest&, HttpResponse&)> GetHandler();
			std::function<void(HttpRequest&, HttpResponse&)> GetRawHandler(std::function<void(WebSocketServer&, const std::string&, const std::vector<unsigned char>&)> clb);
			std::function<void(HttpRequest&, HttpResponse&)> GetScriptHandler(const std::wstring & Url);
			std::map<std::string, std::thread> ServingThreads;
			std::map<std::string, WebSocket> Connections;
			std::string functions;
			void CloseConnection(const std::string& ConnectionId);

			std::atomic_int running;
			std::thread ThreadCleanerThread;
			std::mutex ThreadCleanerMutex;
			std::condition_variable ThreadCleaner_cv;
			void ThreadCleanerClb();
			std::vector<std::string> CleanUpKeys;
		public:
			~WebSocketServer();
			WebSocketServer();
			void AddClientAction(const std::string & Name, std::function<void(PVX::JSON::Item&, const std::string&)> Action);
			void AddClientActionRaw(const std::string Name, std::function<void(const std::vector<unsigned char>&, const std::string&)> Action);
			void AddServerAction(const std::string Name, std::function<void(WebSocketPacket&)> Action);
			
			void AddToGroup(const std::string& GroupName, const std::string & ConnectionId);
			void RemoveFromGroup(const std::string& GroupName, const std::string & ConnectionId);

			std::vector<std::string> GetGroupConnections(const std::string& Name);
			std::vector<std::string> GetConnectionsGroup(const std::string& Name);

			void OnConnect(std::function<void(const std::string&, WebSocket&)> fnc);
			void OnDisconnect(std::function<void(const std::string&)> fnc);
			
			void Send(const std::string & ConnectionId, std::function<void(WebSocketPacket&)> Event);
			void SendGroup(const std::string & GroupName, std::function<void(WebSocketPacket&)> Event);
			void SendAll(std::function<void(WebSocketPacket&)> Event);
			void SendAllExceptOne(const std::string & Id, std::function<void(WebSocketPacket&)> Event);
			void SendGroupExceptOne(const std::string & Id, const std::string & GroupName, std::function<void(WebSocketPacket&)> Event);

			void Run(const std::string& ConnectionId, const std::wstring& Function, const PVX::JSON::Item& Params = nullptr);
			void RunGroup(const std::string& GroupName, const std::wstring& Function, const PVX::JSON::Item& Params = nullptr);
			void RunAll(const std::wstring& Function, const PVX::JSON::Item& Params = nullptr);
			void RunAllExceptOne(const std::string& Id, const std::wstring& Function, const PVX::JSON::Item& Params = nullptr);
			void RunGroupExceptOne(const std::string& Id, const std::string& GroupName, const std::wstring& Function, const PVX::JSON::Item& Params = nullptr);
		};

		class HttpClient {
		public:
			class HttpResponse {
			protected:
				HttpClient::HttpResponse();
				friend class HttpClient;
				std::vector<unsigned char> Data;
				std::wstring Protocol;
			public:
				int StatusCode;
				JSON::Item Json();
				std::vector<unsigned char> Raw();
				std::string Text();
				std::wstring UtfText();
				std::vector<std::pair<std::wstring, std::wstring>> Headers;
			};

			HttpClient();
			HttpClient(const std::string& url);
			HttpClient(const std::wstring& url);

			HttpClient::HttpResponse Get();
			HttpClient::HttpResponse Post(const std::vector<unsigned char> & Data);
			HttpClient::HttpResponse Post(const std::wstring & Data);
			HttpClient::HttpResponse Post(const JSON::Item & Data);

			HttpClient::HttpResponse Get(const std::string& url);
			HttpClient::HttpResponse Post(const std::string& url, const std::vector<unsigned char>& Data);
			HttpClient::HttpResponse Post(const std::string& url, const std::wstring& Data);
			HttpClient::HttpResponse Post(const std::string& url, const JSON::Item& Data);

			HttpClient::HttpResponse Get(const std::wstring& url);
			HttpClient::HttpResponse Post(const std::wstring& url, const std::vector<unsigned char>& Data);
			HttpClient::HttpResponse Post(const std::wstring& url, const std::wstring& Data);
			HttpClient::HttpResponse Post(const std::wstring& url, const JSON::Item& Data);

			HttpClient & OnReceiveHeader(std::function<void(const std::wstring&)> fnc);
			HttpClient & OnReceiveData(std::function<void(const std::vector<unsigned char>&)> fnc);
			std::map<std::wstring, std::wstring> Cookies;
			void Headers(const std::map<std::string, std::wstring>& h);
			UtfHelper& operator[](const std::string& Name);
		protected:
			void Url(const std::wstring & url);
			void Url(const std::string & url);
			std::function<void(const std::wstring&)> onReceiveHeader;
			std::function<void(const std::vector<unsigned char>&)> onReceiveData;
			std::string protocol, domain, query, port;
			std::map<std::string, UtfHelper> headers;
			std::wstring MakeHeader(const char * Verb);
			int Receive(PVX::Network::TcpSocket&, std::vector<std::pair<std::wstring, std::wstring>> & Headers, std::vector<unsigned char> &, std::wstring & Proto, int & Status);
		};
	}
}
#endif