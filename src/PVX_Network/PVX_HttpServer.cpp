#include<PVX_Network.h>
#include<PVX_File.h>
#include<PVX_Encode.h>
#include<PVX_Encrypt.h>
#include<regex>
#include<map>
#include <PVX_Deflate.h>
#include<PVX_String.h>
#include<chrono>

using namespace std::chrono_literals;



namespace PVX {
	namespace Network {
		typedef unsigned char uchar;

		HttpServer::HttpServer() :DefaultRoute{ L"/{Path}", ContentServer() }, Mime{
#include "mime.inl"
		} {}

		HttpServer::HttpServer(const std::wstring & ConfigFile) : DefaultRoute{ L"/{Path}", ContentServer() } {
			auto Config = PVX::IO::LoadJson(ConfigFile.c_str());

			if (auto it = Config.Has(L"Mime"); it)
				for (auto & [Key, Value] : it->getObject())
					Mime[Key] = Value.String();

			if (auto it = Config.Has(L"ContentDir"); it)
				SetDefaultRoute(ContentServer(it->String()));

			if (auto it = Config.Has(L"ResponseHeader"); it)
				for (auto & Value : it->getObject())
					DefaultHeader.push_back({ Value.first, Value.second.String() });
		}

		HttpServer::~HttpServer() {
			WebSocketServers.clear();
		}

		std::wstring MakeRegex(const std::wstring & url) {
			return L"^" + regex_replace(url, std::wregex(LR"regex(\{[^\s\{]*\})regex"), LR"regex(([^\s\?]*))regex") + LR"regex((\?(\S*))?)regex";
		}
		std::wstring MakeRegex2(const std::wstring & url) {
			return regex_replace(url, std::wregex(LR"regex(\{[^\s\{]*\})regex"), LR"regex(\{([\S]*)\})regex");
		}

		std::string ToLower(const std::string & s) {
			std::string ret;
			ret.resize(s.size());
			std::transform(s.begin(), s.end(), ret.begin(), [](wchar_t c) { return c | ('a'^'A'); });
			return ret;
		}

		Route::Route(const std::wstring & url, std::function<void(HttpRequest&, HttpResponse&)> action) : Matcher(MakeRegex(url), std::regex_constants::optimize | std::regex_constants::icase | std::regex_constants::ECMAScript), Action(action) {
			OriginalRoute = url;
			std::wregex r(MakeRegex2(url));
			std::wsmatch m;
			if (std::regex_search(url, m, r))
				for (auto i = 1; i < m.size(); i++)
					Names.push_back(m[i].str());
		}

		int Route::Match(const std::wstring & url, std::map<std::wstring, UtfHelper> & Vars, UtfHelper & Query) {
			std::wsmatch m;
			if (std::regex_search(url, m, Matcher) && m.suffix() == L"" && m.size() >= Names.size() + 1) {
				for (auto i = 0; i < Names.size(); i++)
					Vars[Names[i]] = PVX::Decode::Uri(m[i + 1].str());

				if (m.size() == Names.size() + 3)
					Query = m[m.size() - 1].str();

				return 1;
			}
			return 0;
		}

		int Route::Run(HttpRequest &rq, HttpResponse & rsp) throw() {
			Action(rq, rsp);
			return 0;
		}

		void Route::ResetAction(std::function<void(HttpRequest&, HttpResponse&)> action) {
			Action = action;
		}

		int GetRequest(TcpSocket& s, HttpRequest& http, std::vector<uchar>& Content) {
			int EoH = -1;
			http.Socket = s;
			while (s.Receive(http.RawHeader) > 0 &&
				(EoH = http.RawHeader.find("\r\n\r\n")) == -1);
			if (EoH != -1) {
				size_t contentLength = 0;
				size_t sz = EoH + 4;

				if (http.RawHeader.size() > sz) {
					Content.resize(http.RawHeader.size() - sz);
					memcpy(&Content[0], &http.RawHeader[EoH + 4], Content.size());
				}
				http.RawHeader.resize(sz);

				http = http.RawHeader;
				auto cc = http.Headers.find("content-length");
				if (cc != http.Headers.end()) {
					contentLength = _wtoi(cc->second->c_str());
					Content.reserve(contentLength);

					while (Content.size() < contentLength && s.Receive(Content) > 0);
				}
				return contentLength == Content.size();
			}
			return 0;
		}


		//int GetRequest(TcpSocket & s, HttpRequest & http, std::vector<uchar> & Content) {
		//	int EoH = -1;
		//	while (s.Receive(http.RawHeader) > 0 &&
		//		(EoH = http.RawHeader.find("\r\n\r\n")) == -1);
		//	if (EoH != -1) {
		//		size_t contentLength = 0;
		//		size_t sz = EoH + 4;

		//		if (http.RawHeader.size() > sz) {
		//			Content.resize(http.RawHeader.size() - sz);
		//			memcpy(&Content[0], &http.RawHeader[EoH + 4], Content.size());
		//		}
		//		http.RawHeader.resize(sz);

		//		http = http.RawHeader;
		//		auto cc = http.Headers.find("content-length");
		//		if (cc != http.Headers.end()) {
		//			contentLength = _wtoi(cc->second->c_str());
		//			Content.reserve(contentLength);

		//			while (Content.size() < contentLength && s.Receive(Content) > 0);
		//		}
		//		return contentLength == Content.size();
		//	}
		//	return 0;
		//}
		void HttpServer::Routes(const Route & r) {
			Router.push_back(r);
		}
		void HttpServer::Routes(const std::wstring & Url, std::function<void(HttpRequest&, HttpResponse&)> Action) {
			auto url = Url;
			if (url.front() != L'/')url = L"/" + url;
			Router.push_back({ url, Action });
		}
		void HttpServer::Routes(const std::initializer_list<Route>& routes) {
			for (auto & r : routes) {
				Router.push_back(r);
			}
		}

		void HttpServer::SetDefaultRoute(std::function<void(HttpRequest&, HttpResponse&)> Action) {
			DefaultRoute.ResetAction(Action);
		}

		void HttpServer::Start() {}

		void HttpServer::Stop() {}




		WebSocketServer & HttpServer::CreateWebSocketServer(const std::wstring & url) {
			auto Url = url;
			if (Url.front() != L'/') Url = L"/" + url;

			WebSocketServers.push_back(std::make_unique<WebSocketServer>());
			auto ret = WebSocketServers.back().get();

			Routes(Url + L".js", ret->GetScriptHandler(Url));
			Routes(Url, ret->GetHandler());
			/*ret->ServingThread.push_back(std::thread([ret]() {
				for(;;) {
					if (ret->Connections.size()) {
						for (auto & [connectionId, Socket] : ret->Connections) {

							auto res = Socket.Receive();
							if (res < 0 || Socket.Opcode == WebSocket::Opcode_Close) {
								ret->CloseConnection(connectionId);
								break;
							} else if (res > 0) {
								int type = Socket.Message[0];
								std::string Name;
								size_t sz;
								for (sz = 1; sz < Socket.Message.size() && Socket.Message[sz] != ':'; sz++) Name.push_back(Socket.Message[sz]);
								if (ret->ClientActions.count(Name)) {
									if (type == 'j') {
										JSON::Item params = JSON::jsElementType::Null;
										if (Socket.Message.size() - sz - 1)
											params = PVX::JSON::parse(&Socket.Message[sz + 1], Socket.Message.size() - sz - 1);

										ret->ClientActions[Name](params, connectionId);
									} else if (type == 'b') {
										std::vector<unsigned char> data(Socket.Message.size() - sz - 1);
										memcpy(&data[0], &Socket.Message[sz + 1], Socket.Message.size() - sz - 1);
										ret->ClientActionsRaw[Name](data, connectionId);
									}
								}
							}
						}
					} else {
						std::this_thread::sleep_for(1ms);
					}
				}
			}));*/
			return *ret;
		}

		const std::wstring & HttpServer::GetMime(const std::wstring & extension) const {
			auto f = Mime.find(extension);
			if (f != Mime.end())
				return f->second;
			return Mime.at(L"");
		}

		std::function<void(HttpRequest&, HttpResponse&)> HttpServer::ContentServer(const std::wstring & ContentPath) {
			std::wstring cPath = PVX::IO::wCurrentPath() + ContentPath + ((ContentPath.size() && (ContentPath.back() == L'\\')) ? L"" : L"\\");
			return [this, cPath](HttpRequest & req, HttpResponse& resp) {
				std::wstring path = cPath + std::regex_replace((std::wstring&)req.Variables[L"Path"], std::wregex(L"/"), L"\\");
				if ((resp.StatusCode = resp.Content.BinaryFile(path.c_str())) == 200) {
#ifdef _DEBUG
					printf("Serving File: %ws\n", path.c_str());
#endif
					std::wsmatch Extension;
					std::map<std::wstring, std::wstring>::iterator pMime;
					auto ext = PVX::IO::FileExtension(path);
					if ((pMime = Mime.find(ext)) != Mime.end()) {
						resp[L"Content-Type"] = pMime->second;
					}
					//if (std::regex_search(path, Extension, std::wregex(L"\\.([^\\.]*)")) && Extension[1].matched && (pMime = Mime.find(Extension[1].str())) != Mime.end()) {
					//	resp[L"Content-Type"] = pMime->second;
					//}
				}
			};
		}

		Route HttpServer::ContentPathRoute(const std::wstring & Url, const std::wstring & Path) {
			auto url = Url;
			if (url.front() != L'/')url = L"/" + url;
			return{ url + L"/{Path}", ContentServer(Path) };
		}

		void HttpServer::DefaultRouteForContent(const std::wstring & Path) {
			SetDefaultRoute(ContentServer(Path));
		}

		std::wstring HttpServer::MakeSession() {
			auto now = std::chrono::system_clock::now().time_since_epoch().count();
			auto sid = (std::wstringstream() << now).str();
			Sessions.insert(sid);
			return sid;
		}

		std::wstring HttpServer::StartSession(PVX::Network::HttpRequest & req, PVX::Network::HttpResponse & resp) {
			if (!req.Cookies.count(L"sid") || !Sessions.count(req.Cookies.at(L"sid"))) {
				auto sid = MakeSession();
				resp.ClearCookie(L"sid");
				resp.SetCookie(L"sid", sid);
				req.Cookies[L"sid"] = sid;
				return req.SessionId = sid;
			}
			return req.SessionId = req.Cookies.at(L"sid");
		}

		void HttpServer::BasicAuthentication(std::function<void(const std::wstring&, const std::wstring&)> clb) {
			AddFilter([clb](PVX::Network::HttpRequest& req, PVX::Network::HttpResponse& resp) {
				req.BasicAuthentication(clb);
				return 1;
			});
		}

		int HttpServer::HandleWebToken(HttpRequest& req, HttpResponse& resp) {
			using namespace PVX::Encrypt;
			if (auto k = req.Cookies.find(L"pxx-token"); k!=req.Cookies.end() && k->second.size() > ((32 * 4 + 2) / 3)) {
				auto Token = PVX::Decode::Base64Url(k->second);
				auto Hash = HMAC<SHA256_Algorithm>(TokenKey.data(), TokenKey.size(), Token.data() + 32, Token.size()-32);
				if (!memcmp(Hash.data(), Token.data(), 32)) {
					req.User = PVX::JSON::parse(Token.data() + 32, Token.size() - 32);
				}
			}
			return 1;
		}

		void HttpServer::EnableWebToken(const std::string& Key) {
			using namespace std::placeholders;
			TokenKey = Key;
			AddFilter(std::bind(&HttpServer::HandleWebToken, this, _1, _2));
		}

		int CompressContent(PVX_DataBuilder & Content) {
			Content.SetData(PVX::Compress::Deflate(Content.GetDataVector()));
			return 1;
		}
		static void CompressContent(HttpRequest & http, HttpResponse & r) {
			if (r.Content.GetLength() && http.SouldCompress() && CompressContent(r.Content))
				r[L"Content-Encoding"] = L"deflate";
		}



		int HttpServer::SendResponse(TcpSocket& Socket, HttpRequest & http, HttpResponse& resp) {
			if (resp.SouldCompress) 
				CompressContent(http, resp);
	
			auto ContentLength = resp.Content.GetLength();
			for (auto & s : resp.Streams) 
				ContentLength += s.Size;


			resp.SendHeader(ContentLength);

			//PVX_DataBuilder Response;
			//Response << StatusCodes[resp.StatusCode];
			//resp.Headers[L"date"] = GetDate();


			//if (ContentLength) {
			//	wchar_t tmp[128];
			//	_ui64tow_s(ContentLength, tmp, 128, 10);
			//	resp.Headers[L"content-length"] = tmp;
			//}

			//for (auto & h : resp.Headers)
			//	Response << h.first << ": " << h.second << "\r\n";

			//for(auto & h : resp.MoreHeaders)
			//	Response << h.Name << ": " << h.Value << "\r\n";

			//Response << "\r\n";


			//if (Socket.Send(Response.GetDataVector()) < 0)return 1;
			if (resp.Content.GetLength() && Socket.SendFragmented(resp.Content.GetDataVector()) < resp.Content.GetLength()) return 1;
			for (auto & s : resp.Streams) {
				while (s.Func(Socket));
			}

			return 1;
		}

		void HttpServer::SetDefaultHeader(HttpResponse & http) {
			http.Server = this;
			for (auto h : DefaultHeader)
				http[h.Name] = h.Value;
		}

		void HttpServer::AddFilter(std::function<int(HttpRequest&, HttpResponse&)> Filter) {
			Filters.push_back(Filter);
		}
		std::function<void(TcpSocket)> HttpServer::GetHandler() {
			return [this](TcpSocket Socket) {
				HttpRequest Request;
				while (GetRequest(Socket, Request, Request.RawContent)) {
					//if (Request.Method=="OPTIONS") {
					//	HttpResponse r;
					//	r.StatusCode = 405;
					//	SendResponse(Socket, Request, r);
					//}

					for (auto & r : Router) {
						if (r.Match(Request.QueryString, Request.Variables, Request.Get)) {
							HandleRequest(Socket, Request, r);
							return;
						}
					}

					if (DefaultRoute.Match(Request.QueryString, Request.Variables, Request.Get)) {
						HandleRequest(Socket, Request, DefaultRoute);
						return;
					}

					HttpResponse r;
					r.StatusCode = 500;
					SendResponse(Socket, Request, r);
				}
			};
		}
	}
}