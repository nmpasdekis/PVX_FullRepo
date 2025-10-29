#ifndef __PVX_NETWORK_H__
#define __PVX_NETWORK_H__

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
#include <PVX_File.h>
#include <shared_mutex>
#include <atomic>
#include <filesystem>

typedef uint64_t SOCKET_type;

// C:\Users\PerVertEX\Google Drive\C++ Projects 2016\Libraries64\OpenSSL\bin64

namespace PVX {
	namespace Network {
		class HttpResponse;
		class WebSocket;
		class HttpRequest;
		class HttpServer;
		class WebSocketServer;
		class WebSocketPacket;

		enum class TcpSocketOption {
			NoDelay = 0x0001,
			Expedited_1122 = 0x0002,
			KeepAlive = 3,
			MaxSeg = 4,
			MaxRT = 5,
			StdUrg = 6,
			NoUrg = 7,
			AtMark = 8,
			NoSynRetries = 9,
			Timestamps = 10,
			Offload_Preference = 11,
			Congestion_Algorithm = 12,
			Delay_Fin_Ack = 13,
			MaxRTMS = 14,
			FastOpen = 15,
			KeepCnt = 16,
			KeepIdle = 3,
			KeepIntvl = 17,
			Fail_Connect_On_ICMP_Error = 18,
			ICMP_Error_Info = 19
		};
		enum class SocketOption {
			Debug = 0x0001,
			AcceptConn = 0x0002,
			ReuseAddr = 0x0004,
			KeepAlive = 0x0008,
			DontRoute = 0x0010,
			Broadcast = 0x0020,
			UseLoopback = 0x0040,
			Linger = 0x0080,
			OobinLine = 0x0100,

			SNDBUF = 0x1001,          /* send buffer size */
			RCVBUF = 0x1002,         /* receive buffer size */
			SNDLOWAT = 0x1003,         /* send low-water mark */
			RCVLOWAT = 0x1004,         /* receive low-water mark */
			SNDTIMEO = 0x1005,         /* send timeout */
			RCVTIMEO = 0x1006,         /* receive timeout */
			Error = 0x1007,         /* get error status and clear */
			TYPE = 0x1008,         /* get socket type */
		};

		class TcpSocket {
		public:
			TcpSocket();
			int Connect(const char* Url, const char* Port);
			int Send(const void* Data, size_t Size);
			size_t SendFragmented(const std::vector<unsigned char>& data, size_t FragmentSize = 0x00800000);
			inline int Send(const std::vector<uint8_t>& Data) { return Send(Data.data(), Data.size()); }
			inline int Send(const std::string& Data) { return Send(Data.data(), Data.size()); }
			int64_t Receive(void* Data, size_t Size);
			int64_t Receive(std::vector<uint8_t>& Data);
			int64_t Receive(std::string& Data);
			std::vector<uint8_t> Receive();

			int64_t ReceiveAsync(void* data, size_t Size);
			int64_t ReceiveAsync(std::vector<uint8_t>& data);

			int CanRead();
			int CanReadAsync();
			int CanReadAsync(uint32_t ms);
			void Disconnect();
			bool IsConnected();

			template<typename T>
			int SetOption(TcpSocketOption Op, const T& Value) {
				return SetOption(Op, &Value, sizeof(T));
			}
			int SetOption(TcpSocketOption Op, const void* Value, int ValueSize);

			template<typename T>
			int SetOption(SocketOption Op, const T& Value) {
				return SetOption(Op, &Value, sizeof(T));
			}
			int SetOption(SocketOption Op, const void* Value, int ValueSize);

			// Internal Use Only
			template<typename T> T& GetInternalData() { return *(T*)SocketData.get(); }
		protected:
			TcpSocket(uint32_t*, const void*);
			TcpSocket(uint32_t*, const void*, std::function<void(void*)> deleter);
			std::shared_ptr<void> SocketData;
			friend class TcpServer;
		};

		class TcpServer {
		public:
			TcpServer(const char* Port = "80", int ThreadCount = 0);
			TcpServer(std::function<void(TcpSocket)> clb, const char* Port = "80", int ThreadCount = 0);
			TcpServer(std::function<void(TcpSocket)> clb, std::function<void(TcpSocket&)> OnConnect, const char* Port = "80", int ThreadCount = 0);
			~TcpServer();
			void Serve(std::function<void(TcpSocket)> clb, std::function<void(TcpSocket&)> OnConnect = nullptr);
			void Stop();
		protected:
			void init(int ThreadCount);
			std::atomic_bool Running;
			std::atomic_int Working = 0;
			uint32_t* ServingSocket;
			std::unique_ptr<std::thread> ServingThread;
			std::vector<std::thread> Workers;
			std::mutex TaskMutex, SocketGuard;
			std::condition_variable TaskAdded, TaskEnded;
			std::queue<std::function<void()>> Tasks;
			std::set<uint32_t*> OpenSockets;
		};

		class UtfHelper {
			std::wstring Text;
		public:
			UtfHelper() {};
			UtfHelper(const std::wstring &);
			UtfHelper(const wchar_t *);
			UtfHelper(const std::vector<unsigned char> &);
			UtfHelper(const std::string &);
			UtfHelper& operator=(const std::wstring&);
			UtfHelper& operator=(const wchar_t*);
			std::wstring * operator->();
			std::wstring & operator()();
			operator std::wstring&();
			operator std::wstring() const;
		};

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
			std::wstring QueryString;
			UtfHelper Get;
			std::map<std::string, UtfHelper> Headers;
			HttpRequest();
			//HttpRequest(const std::string & s);
			HttpRequest & operator=(const std::string & s);
			std::vector<unsigned char> RawContent;
			std::wstring HasHeader(const std::string & h) const;
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
			int64_t operator()(const std::wstring & Name);
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
			UtfHelper & operator[](const std::wstring &);
			PVX_DataBuilder Content;
			void Json(const PVX::JSON::Item & json);
			void Redirect(const std::wstring & Location, int Status = 303);
			void Json(const std::wstring & json);
			void Json(const std::string & json);
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
			int StreamFile(const std::filesystem::path& Filename, size_t BufferSize = 0x00800000, const std::string & Mime = "");
			int StreamFile(HttpRequest& Request, const std::wstring& Filename, size_t BufferSize = 0x00800000);
			int ServeFile(const std::filesystem::path& Filename, const std::string & Mime = "");

			int SingleRangeFile(size_t Offset, size_t Size, const std::filesystem::path& Filename, size_t FragmentSize=0x00100000);

			void Download(const std::filesystem::path& Filename);

			// return true if More data sould me sent
			void StreamRaw(size_t Size, std::function<void(TcpSocket & Socket)> fnc);

			void SetCookie(const std::wstring & Name, const std::wstring & Value, const std::wstring& Path = L"");
			void ClearCookie(const std::wstring & Name, const std::wstring& Path = L"");
			int SendHeader(size_t ContentLength = 0);
			void AllowOrigin(HttpRequest& req, const std::set<std::wstring>& Allow = {});

			void MakeWebToken(const PVX::JSON::Item& User);


			int StatusCode = 200;
			bool SouldCompress = true;
			bool Handled = false;

		protected:
			TcpSocket Socket;
			size_t RangeSize;

			HttpServer * Server;
			struct myStream {
				template<typename T>
				inline myStream(size_t sz, T&& f) : Size{ sz }, Func{ std::forward<T>(f) } {}
				size_t Size;
				std::function<void(TcpSocket&)> Func;
			};
			std::vector<myStream> Streams;
			std::unordered_map<std::wstring, UtfHelper> Headers;
			std::unordered_map<std::wstring, std::wstring> MoreHeaders;

			friend class HttpServer;
			friend class Controller;
		};

		class Route {
		public:
			Route(const std::wstring& url, std::function<void(HttpRequest&, HttpResponse&)> Action);
			Route(const std::wregex& url, const std::vector<std::wstring>& Names, std::function<void(HttpRequest&, HttpResponse&)> Action);
			int Match(const std::wstring & url, std::map<std::wstring, UtfHelper> & Vars, UtfHelper & Query);
			int Run(HttpRequest &, HttpResponse &);
			void ResetAction(std::function<void(HttpRequest&, HttpResponse&)> Action);
		private:
			//std::wstring OriginalRoute;
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
			HttpServer(TcpServer& srv);
			~HttpServer();

			std::function<void(TcpSocket)> GetHandler();
			operator std::function<void(TcpSocket)>() {
				return GetHandler();
			}

			void Routes(const Route & routes);

			void Routes(std::wregex url, const std::vector<std::wstring>& Names, std::function<void(HttpRequest&, HttpResponse&)> Action);
			inline void Routes(std::wregex url, const std::vector<std::wstring>& Names, std::function<void(HttpResponse&)> Action) { Routes(url, Names, [Action](HttpRequest&, HttpResponse& resp) { Action(resp); }); }
			inline void Routes(std::wregex url, const std::vector<std::wstring>& Names, std::function<void(HttpRequest&)> Action) { Routes(url, Names, [Action](HttpRequest& req, HttpResponse&) { Action(req); }); }
			inline void Routes(std::wregex url, const std::vector<std::wstring>& Names, std::function<void()> Action) { Routes(url, Names, [Action](HttpRequest&, HttpResponse&) { Action(); }); }

			void Routes(const std::wstring& url, std::function<void(HttpRequest&, HttpResponse&)> Action);
			inline void Routes(const std::wstring& url, std::function<void(HttpResponse&)> Action) { Routes(url, [Action](HttpRequest&, HttpResponse& resp) { Action(resp); }); }
			inline void Routes(const std::wstring& url, std::function<void(HttpRequest&)> Action) { Routes(url, [Action](HttpRequest& req, HttpResponse&) { Action(req); }); }
			inline void Routes(const std::wstring& url, std::function<void()> Action) { Routes(url, [Action](HttpRequest&, HttpResponse&) { Action(); }); }

			inline void Routes(const std::string& utf8_url, std::function<void(HttpRequest&, HttpResponse&)> Action) { Routes(PVX::Decode::UTF(utf8_url), std::move(Action)); }
			inline void Routes(const std::string& utf8_url, std::function<void(HttpRequest&)> Action) { Routes(PVX::Decode::UTF(utf8_url), [Action](HttpRequest& req, HttpResponse&) { Action(req); }); }
			inline void Routes(const std::string& utf8_url, std::function<void(HttpResponse&)> Action) { Routes(PVX::Decode::UTF(utf8_url), [Action](HttpRequest&, HttpResponse& resp) { Action(resp); }); }
			inline void Routes(const std::string& utf8_url, std::function<void()> Action) { Routes(PVX::Decode::UTF(utf8_url), [Action](HttpRequest&, HttpResponse&) { Action(); }); }
			
			void Routes(const std::initializer_list<Route> & routes);

			void AddFilter(std::function<int(HttpRequest&, HttpResponse&)> Filter);

			void SetDefaultRoute(std::function<void(HttpRequest&, HttpResponse&)> Action);
			void Start();
			void Stop();
			WebSocketServer & CreateWebSocketServer(const std::wstring& Url);
			WebSocketServer & CreateWebSocketServer(const std::wstring & Url, std::function<void(WebSocketServer&, const std::string&, const std::vector<unsigned char>&)> clb);
			const std::string & GetMime(const std::string & extension) const;

			// Route must contain {Path} Variable
			std::function<void(HttpRequest&, HttpResponse&)> ContentServer(const std::filesystem::path& ContentPath = L"");
			std::function<void(HttpRequest&, HttpResponse&)> HtmlFileRoute(const std::filesystem::path& Filename) {
				return [Filename](HttpRequest& req, HttpResponse& resp) {
					resp.ServeFile(Filename, "text/html");
				};
			};

			inline void ContentRoute(const std::wstring & Url, const std::filesystem::path& Path) {
				auto url = Url;
				if (url.size() && url.front() != L'/')url = L"/" + url;
				Routes({ url + L"/{Path}", ContentServer(Path) });
			}
			inline void ContentRoutes(const std::filesystem::path& contentPath) {
				namespace fs = std::filesystem;
				fs::path path;
				if (contentPath.is_relative()) path = fs::current_path() / contentPath;
				else path = contentPath;
				for (const auto &p : fs::directory_iterator(path)) {
					if (p.is_directory()) {
						ContentRoute(L"/" + p.path().filename().wstring(), p);
					}
				}
			}
			inline void DefaultRouteForContent(const std::filesystem::path& Path) {
				SetDefaultRoute(ContentServer(Path));
			}
			inline void DefaultHtml(const std::filesystem::path& Filename) {
				SetDefaultRoute(HtmlFileRoute(Filename));
			}
			inline void ServeFile(const std::wstring& Url, const std::filesystem::path& File) {
				Routes(Url, [File](HttpResponse& resp) {
					resp.ServeFile(File);
				});
			}

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
			std::map<std::string, std::string> Mime;
			//std::vector<SimpleTuple> DefaultHeader;
			std::unordered_map<std::wstring, std::wstring>  DefaultHeader;
			std::vector<std::function<int(HttpRequest&, HttpResponse&)>> Filters;
			std::set<std::wstring> Sessions;
			std::string TokenKey;
			std::atomic_bool Running = true;
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
				HttpResponse();
				friend class HttpClient;
				std::vector<unsigned char> Data;
				std::wstring Protocol;
				std::wstring ct;
				TcpSocket Socket;
			public:
				int StatusCode;
				JSON::Item Json() const;
				std::vector<unsigned char> Raw() const;
				std::string Text() const;
				std::wstring UtfText() const;
				WebSocket WebSocket() const;
				std::vector<std::pair<std::wstring, std::wstring>> Headers;
				const std::wstring& ContentType() const;
				HttpResponse &Then(std::function<void(const PVX::JSON::Item &)> json, std::function<void(HttpResponse&)> Error);
				HttpResponse &Then(std::function<void(const PVX::JSON::Item &)> json);
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

			HttpClient& OnReceiveHeader(std::function<void(const std::wstring&)> fnc);
			HttpClient& OnReceiveData(std::function<void(const std::vector<unsigned char>&)> fnc);
			HttpClient& OnConnect(std::function<void(TcpSocket&)> clb);
			std::map<std::wstring, std::wstring> Cookies;

			HttpClient& Headers(const std::unordered_map<std::string, std::wstring>& h);
			HttpClient& Headers_Raw(const std::unordered_map<std::string, std::wstring>& h);
			HttpClient& HeadersAll(const std::unordered_map<std::string, std::wstring>& h);
			HttpClient& HeadersAll_Raw(const std::unordered_map<std::string, std::wstring>& h);

			HttpClient& Headers(const std::unordered_map<std::string, std::string>& h);
			HttpClient& Headers_Raw(const std::unordered_map<std::string, std::string>& h);
			HttpClient& HeadersAll(const std::unordered_map<std::string, std::string>& h);
			HttpClient& HeadersAll_Raw(const std::unordered_map<std::string, std::string>& h);

			UtfHelper& operator[](const std::string& Name);

			const std::string& Domain() const { return domain; }
			const std::string& Port() const { return port; }
			const std::string& Protocol() const { return protocol; }

			HttpClient& BasicAuth(const std::string Username, const std::string& Password);
			HttpClient& BasicAuth(const std::wstring Username, const std::wstring& Password);
		protected:
			void urlHelper(const std::string_view& url);
			void DomainHelper(std::string_view url);
			void Url(const std::wstring & url);
			void Url(const std::string & url);
			std::wstring MakeHeader(const char * Verb);
			int Receive(PVX::Network::TcpSocket&, std::vector<std::pair<std::wstring, std::wstring>> & Headers, std::vector<unsigned char> &, std::wstring & Proto, int & Status, std::wstring& ct);

			std::function<void(TcpSocket&)> onConnect;
			std::function<void(const std::wstring&)> onReceiveHeader;
			std::function<void(const std::vector<unsigned char>&)> onReceiveData;
			std::string protocol = "http", port = "80", domain, path;
			std::unordered_map<std::string, UtfHelper> headers;
		};

		std::string BasicAuthentication(const std::string& Username, const std::string& Password);

		inline PVX::JSON::Item ls(const std::filesystem::path& path) {
			return {
				//{ L"Path", path.wstring() },
				{ L"Directories", PVX::IO::SubDir(path) },
				{ L"Files", PVX::IO::Dir(path) },
			};
		}
	}
}
#endif