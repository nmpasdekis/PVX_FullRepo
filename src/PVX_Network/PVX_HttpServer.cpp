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
#include<iostream>

using namespace std::chrono_literals;



namespace PVX {
	namespace Network {
#ifdef __linux
		inline struct tm* localtime_s(struct tm* buf, const time_t* timer) {
			*buf = *localtime(timer);
			return buf;
		}
#endif
		typedef unsigned char uchar;

		HttpServer::HttpServer() :DefaultRoute{ L"/{Path}", ContentServer() }, Mime{
#include "mime.inl"
		} {}
		HttpServer::HttpServer(TcpServer& srv): HttpServer{} {
			srv.Serve(*this);
		}

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
			while(true) {
				rcvSize = s.Receive(buffer);
				if(rcvSize<=0) 
					break;
				EoH = FindEnd(buffer);
				if(EoH != -1) 
					break;
			}
			//while ((rcvSize = s.Receive(buffer)) > 0 && (EoH = FindEnd(buffer)) == -1);

			if (EoH != -1) {
				size_t contentLength = 0;
				size_t sz = EoH + 4;

				Request.RawHeader.resize(sz);
				std::memcpy(Request.RawHeader.data(), buffer.data(), sz);
				RemoveFront(buffer, sz);

				Request = Request.RawHeader;

				if (auto c = Request.Headers.find("connection"); c != Request.Headers.end() && c->second() == L"keep-alive") {
					int ka = 1;
					s.SetOption(PVX::Network::SocketOption::KeepAlive, &ka, sizeof(int));
				}

				
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

		Route& HttpServer::Routes(const Route & r) {
			return Router.emplace_back(r);
		}
		Route& HttpServer::Routes(const std::wstring & Url, std::function<void(HttpRequest&, HttpResponse&)> Action) {
			auto url = Url;
			if (url.front() != L'/')url = L"/" + url;
			return Router.emplace_back( url, Action);
		}
		Route& HttpServer::Routes(std::wregex url, const std::vector<std::wstring>& Names, std::function<void(HttpRequest&, HttpResponse&)> Action) {
			return Router.emplace_back(url, Names, Action);
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
			if (auto f = Mime.find(extension); f != Mime.end())
				return f->second;
			return Mime.at("");
		}

		std::function<void(HttpRequest&, HttpResponse&)> HttpServer::ContentServer(const std::filesystem::path& ContentPath, size_t thresshold) {
			namespace io = PVX::IO;
			namespace fs = std::filesystem;

			auto cPath = fs::current_path() / ContentPath;

			return [cPath, thresshold](HttpRequest& req, HttpResponse& resp) {
				auto path = cPath / (std::wstring&)req.Variables[L"Path"];

				if (!path.is_absolute()) {
					resp.StatusCode = 403;
					return;
				}
				io::FileExists(path);

				if (!fs::exists(path)) { resp.StatusCode = 404; return; }

				if (fs::file_size(path) < thresshold) {
					resp.ServeFile(path);
				} else {
					resp.StreamFile(req, path);
				}
			};
		}

//#define isSpace(c) (c==' '||c=='\t'||c=='\n'||c=='\r')

		void HttpServer::Bundle(const std::wstring& name, const std::initializer_list<std::filesystem::path>& files) {
			auto bPath = std::filesystem::path(name).stem();
			
			if (!std::filesystem::exists(bPath)) {
				std::stringstream bundle;
				for (const auto& f : files) {
					bundle << PVX::IO::ReadText(f) << "\r\n";
				}
				PVX::IO::Write(bPath, bundle.str());
				Routes(name, [b = bundle.str()](HttpResponse& resp) {
					resp.Content.WriteText(b);
				});
			}
			else Routes(name, [b = PVX::IO::ReadText(bPath)](HttpResponse& resp) {
				resp.Content.WriteText(b);
			});
		}

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

		std::string BasicAuthentication(const std::string& Username, const std::string& Password) {
			return "basic " + PVX::Encode::Base64Url(Username + ":" + Password);
		}
		std::string MakeJavaWebToken(const std::vector<uint8_t>& secret, const std::initializer_list<std::pair<std::wstring, std::variant<std::wstring, int64_t>>>& data, int ExpireMinutes) {
			using namespace PVX::Encrypt;
			auto header = PVX::Encode::Base64Url(R"({"alg":"HS256","typ":"JWT"})", true);
			std::wstringstream plData;

			time_t t = time(0);

			plData << L"{";
			for (auto& d : data) {
				plData << L'"' << d.first << LR"(":)";
				if (d.second.index() == 0)
					plData << L'"' << std::get<0>(d.second) << LR"(",)";
				else
					plData << std::get<1>(d.second) << L',';
			}
			if(ExpireMinutes > 0) {
				plData << LR"("iat":)" << t << LR"(,"exp":)" << (t + ExpireMinutes * 60);
			} else {
				plData << LR"("iat":)" << t;
			}

			plData << L"}";
			auto payload = PVX::Encode::Base64Url(PVX::Encode::UtfString(plData.str()), true);
			auto message = header + "." + payload;

			auto signature = HMAC<SHA256_Algorithm>(secret, message);

			return message + "." + PVX::Encode::Base64Url(signature.data(), signature.size(), true);
		}

		PVX::JSON::Item ValidateJavaWebToken(const std::vector<uint8_t>& secret, const std::string& token, bool expires) {
			using namespace PVX::Encrypt;
			auto sp = PVX::String::Split(token, ".");
			if(sp.size() == 3) {
				auto message = sp[0] + '.' + sp[1];
				auto signature = PVX::Encode::Base64Url(HMAC<SHA256_Algorithm>(secret, message), true);
				if(signature == sp[2]) {
					auto data = PVX::JSON::parse(PVX::Decode::Base64Url(sp[1]));
					if(!expires || data["exp"].Int64() > time(0))
						return data;
				}
			}
			return nullptr;
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

		Route& HttpServer::FileServer(const std::wstring & Url, const std::initializer_list<std::filesystem::path>& roots) {
			namespace fs = std::filesystem;
			std::wstring url = Url.back() == L'/' ? Url.substr(0, Url.length() - 1) : Url;
			std::unordered_map<fs::path, fs::path> Roots;

			for (const auto& r : roots) {
				auto name = r.filename();
				Roots[name] = r.parent_path();
			}

			return this->Routes(url + L"/{Path}", [Roots](HttpRequest& req, HttpResponse& resp) {
				std::filesystem::path p = req.Variables[L"Path"]();
				auto r = p;
				while (r.has_parent_path()) r = r.parent_path();
				if (Roots.count(r)) {
					auto fn = Roots.at(r) / p;
					if (fs::is_directory(fn)) {
						auto ls = PVX::Network::ls(fn);
						//ls["Files"] = ls["Files"].filter([](const PVX::JSON::Item& file) {
						//	fs::path filename = file.String();
						//	return (filename.extension() == ".mp4");
						//});
						resp.Json(ls);
					}
					else
						resp.StreamFile(req, fn);
				}
			});
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
		void HttpServer::AddNamedFilter(const std::string& Name, std::function<int(HttpRequest&, HttpResponse&, const PVX::JSON::Item&)> Filter) {
			NamedFilters.emplace(Name, Filter);
		}
		std::function<void(TcpSocket)> HttpServer::GetHandler() {
			return [this](TcpSocket Socket) {
#ifndef __linux
				SetThreadDescription(GetCurrentThread(), L"Http Server");
#endif

				// Catch Memory Exceptions (maybe)
				signal(SIGSEGV, [](int Signal) {
					throw "Access Violation";
				});

				HttpRequest Request;
				std::vector<uint8_t> Buffer;
				while (Running && GetRequest(Buffer, Socket, Request, Request.RawContent)) {

					for (auto & r : Router) {
						if (r.Match(Request.QueryString, Request.Variables, Request.Get)) {
							//std::wcout << Request.QueryString << "\n";
							HandleRequest(Socket, Request, r);
							return;
						}
					}

					if (DefaultRoute.Match(Request.QueryString, Request.Variables, Request.Get)) {
						HandleRequest(Socket, Request, DefaultRoute);
						return;
					}

					HttpResponse resp;
					resp.StatusCode = 500;
					SendResponse(Socket, Request, resp);
				}
			};
		}
	}
}