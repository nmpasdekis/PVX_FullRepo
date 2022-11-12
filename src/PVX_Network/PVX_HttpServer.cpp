#define NOMINMAX

#include<PVX_Network.h>
#include<PVX_File.h>
#include<PVX_Encode.h>
#include<PVX_Encrypt.h>
#include<regex>
#include<map>
#include<PVX_Deflate.h>
#include<PVX_String.h>
#include<chrono>
#include<signal.h>
#include<PVX.inl>
#include<memory>

using namespace std::chrono_literals;



namespace PVX {
	namespace Network {
		typedef unsigned char uchar;

		HttpServer::HttpServer() :DefaultRoute{ L"/{Path}", ContentServer() }, Mime{
#include "mime.inl"
		} {}

		HttpServer::~HttpServer() {
			Running = false;
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

		Route::Route(const std::wstring & url, std::function<void(HttpRequest&, HttpResponse&)> action) : 
			Action(action), Matcher(MakeRegex(url), std::regex_constants::optimize | std::regex_constants::icase | std::regex_constants::ECMAScript) {
			//OriginalRoute = url;
			std::wregex r(MakeRegex2(url));
			std::wsmatch m;
			if (std::regex_search(url, m, r))
				for (auto i = 1; i < m.size(); i++)
					Names.push_back(m[i].str());
		}

		Route::Route(const std::wregex& url, const std::vector<std::wstring>& Names, std::function<void(HttpRequest&, HttpResponse&)> action) :
			Action(action), Names{ Names }, Matcher{ url } {}

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

		int Route::Run(HttpRequest &rq, HttpResponse & rsp) {
			Action(rq, rsp);
			return 0;
		}

		void Route::ResetAction(std::function<void(HttpRequest&, HttpResponse&)> action) {
			Action = action;
		}

		int64_t FindEnd(std::vector<uint8_t>& buffer) {
			for (auto i = 0; i < int64_t(buffer.size()) - 3; i++) {
				if (!std::memcmp(&buffer[i], "\r\n\r\n", 4)) 
					return i;
			}
			return -1;
		}

		void RemoveFront(std::vector<uint8_t>& buff, size_t sz) {
			if (sz == buff.size()) {
				buff.clear();
			} else {
				for (size_t i = sz, j = 0; i< buff.size(); i++, j++) {
					buff[j] = buff[i];
				}
				buff.resize(buff.size() - sz);
			}
		}

		int GetRequest(std::vector<uint8_t>& buffer, TcpSocket& s, HttpRequest& Request, std::vector<uint8_t>& Content) {
			Request.Socket = s;
			int rcvSize;
			int EoH = -1;
			while ((rcvSize = s.Receive(buffer)) > 0 && (EoH = FindEnd(buffer)) == -1);

			if (EoH != -1) {
				size_t contentLength = 0;
				size_t sz = EoH + 4;

				Request.RawHeader.resize(sz);
				std::memcpy(Request.RawHeader.data(), buffer.data(), sz);
				RemoveFront(buffer, sz);

				Request = Request.RawHeader;
				
				if (auto cc = Request.Headers.find("content-length"); cc != Request.Headers.end() && (contentLength = std::stoll(cc->second))) {
					Content.reserve(contentLength);
					if (int more = std::min(contentLength, buffer.size());  more) {
						Content.resize(more);
						std::memcpy(Content.data(), buffer.data(), more);
						RemoveFront(buffer, more);
					}
					while (Content.size() < contentLength && s.Receive(Content) > 0);
					if (Content.size() > contentLength) {
						buffer.resize(Content.size() - contentLength);
						std::memcpy(buffer.data(), &Content[contentLength], Content.size() - contentLength);
						Content.resize(contentLength);
					}
				}
				return 1;
			}
			return 0;
		}

		int GetRequest2(TcpSocket& s, HttpRequest& http, std::vector<uchar>& Content) {
			int EoH = -1;
			http.Socket = s;
			int rcvSize;
			while ((rcvSize = s.Receive(http.RawHeader)) > 0 &&
				(EoH = http.RawHeader.find("\r\n\r\n")) == -1);
			if (EoH != -1) {
				size_t contentLength = 0;
				size_t sz = EoH + 4;

				if (http.RawHeader.size() > sz) {
					Content.resize(http.RawHeader.size() - sz);
					std::memcpy(&Content[0], &http.RawHeader[EoH + 4], Content.size());
				}
				http.RawHeader.resize(sz);

				http = http.RawHeader;
				auto cc = http.Headers.find("content-length");
				if (cc != http.Headers.end()) {
					contentLength = std::stoll(cc->second);
					Content.reserve(contentLength);

					while (Content.size() < contentLength && s.Receive(Content) > 0);
				}
				return contentLength == Content.size();
			}
			return 0;
		}

		void HttpServer::Routes(const Route & r) {
			Router.push_back(r);
		}
		void HttpServer::Routes(const std::wstring & Url, std::function<void(HttpRequest&, HttpResponse&)> Action) {
			auto url = Url;
			if (url.front() != L'/')url = L"/" + url;
			Router.push_back({ url, Action });
		}
		void HttpServer::Routes(std::wregex url, const std::vector<std::wstring>& Names, std::function<void(HttpRequest&, HttpResponse&)> Action) {
			Router.push_back({ url, Names, Action });
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

			auto ret = WebSocketServers.emplace_back(std::make_unique<WebSocketServer>()).get();
			//WebSocketServers.push_back(std::make_unique<WebSocketServer>());
			//auto ret = WebSocketServers.back().get();

			Routes(Url + L".js", ret->GetScriptHandler(Url));
			Routes(Url, ret->GetHandler());
			return *ret;
		}

		WebSocketServer& HttpServer::CreateWebSocketServer(const std::wstring& url, std::function<void(WebSocketServer&, const std::string&, const std::vector<unsigned char>&)> clb) {
			auto Url = url;
			if (Url.front() != L'/') Url = L"/" + url;

			WebSocketServers.push_back(std::make_unique<WebSocketServer>());
			auto ret = WebSocketServers.back().get();

			Routes(Url, ret->GetRawHandler(clb));
			return *ret;
		}

		const std::string & HttpServer::GetMime(const std::string & extension) const {
			auto f = Mime.find(extension);
			if (f != Mime.end())
				return f->second;
			return Mime.at("");
		}

		std::function<void(HttpRequest&, HttpResponse&)> HttpServer::ContentServer(const std::filesystem::path& ContentPath) {
			namespace io = PVX::IO;
			namespace fs = std::filesystem;

			auto cPath = fs::current_path() / ContentPath;

			return [cPath](HttpRequest& req, HttpResponse& resp) {
				auto path = cPath / (std::wstring&)req.Variables[L"Path"];

				if (!path.is_absolute()) {
					resp.StatusCode = 403;
					return;
				}
				io::FileExists(path.wstring());

				if (!fs::exists(path)) { resp.StatusCode = 404; return; }

				if (fs::file_size(path) < 1024 * 1024) {
					resp.ServeFile(path.wstring());
				} else {
					resp.StreamFile(req, path.wstring());
				}
			};
		}

		//std::function<void(HttpRequest&, HttpResponse&)> HttpServer::ContentServer(const std::wstring & ContentPath) {
		//	namespace io = PVX::IO;
		//	std::wstring cPath = PVX::IO::wCurrentPath() + 
		//		(ContentPath[0] != io::sep ? io::sepString : L"") +
		//		ContentPath + 
		//		((ContentPath.size() && (ContentPath.back() == io::sep)) ? L"" : io::sepString);
		//	return [this, cPath](HttpRequest & req, HttpResponse& resp) {
		//		auto path = [&] {
		//			if constexpr (io::sep=='\\') return std::filesystem::path(cPath + (std::wstring&)req.Variables[L"Path"]).make_preferred().wstring();
		//			else return cPath + (std::wstring&)req.Variables[L"Path"];
		//		}(); 
		//		if (path.find(L"..") != std::wstring::npos) {
		//			resp.StatusCode = 403;
		//			return;
		//		}
		//		if ((resp.StatusCode = resp.Content.BinaryFile(path.c_str())) == 200) {
		//			std::wsmatch Extension;
		//			std::map<std::wstring, std::wstring>::iterator pMime;
		//			auto ext = PVX::IO::FileExtension(path);
		//			if ((pMime = Mime.find(ext)) != Mime.end()) {
		//				resp[L"Content-Type"] = pMime->second;
		//			}
		//		}
		//	};
		//}

		//Route HttpServer::ContentPathRoute(const std::wstring & Url, const std::wstring & Path) {
		//	auto url = Url;
		//	if (url.front() != L'/')url = L"/" + url;
		//	return{ url + L"/{Path}", ContentServer(Path) };
		//}

		//void HttpServer::DefaultRouteForContent(const std::wstring & Path)

		std::wstring HttpServer::MakeSession() {
			auto now = std::chrono::system_clock::now().time_since_epoch().count();
			auto sid = std::to_wstring(now);
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
			if (auto k = req.Cookies.find(L"pvx-token"); k!=req.Cookies.end() && k->second.size() > ((32 * 4 + 2) / 3)) {
				auto Token = PVX::Decode::Base64Url(k->second);
				auto Hash = HMAC<SHA256_Algorithm>(TokenKey.data(), TokenKey.size(), Token.data() + 32 + 1, Token.size() - 32 - 1);
				if (!std::memcmp(Hash.data(), Token.data(), 32)) {
					req.User = PVX::JSON::parse(Token.data() + 32 + 1, Token.size() - 32 - 1);
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
				s.Func(Socket);
			}

			return 1;
		}

		void HttpServer::SetDefaultHeader(HttpResponse & http) {
			http.Server = this;
			for (auto & [Name, Value] : DefaultHeader)
				http[Name] = Value;
		}

		void HttpServer::AddFilter(std::function<int(HttpRequest&, HttpResponse&)> Filter) {
			Filters.push_back(Filter);
		}
		std::function<void(TcpSocket)> HttpServer::GetHandler() {
			return [this](TcpSocket Socket) {

				// Catch Memory Exceptions (maybe)
				signal(SIGSEGV, [](int Signal) {
					throw "Access Violation";
				});

				HttpRequest Request;
				std::vector<uint8_t> Buffer;
				while (Running && GetRequest(Buffer, Socket, Request, Request.RawContent)) {
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