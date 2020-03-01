#include <PVX_Network.h>
#include <PVX_String.h>
#include <PVX.inl>
#include <PVX_Encode.h>
#include <PVX_Regex.h>

namespace PVX::Network {
	HttpRequest::HttpRequest() {}
	HttpRequest::HttpRequest(const std::string & s) {
		SetHeader(s);
	}
	HttpRequest & HttpRequest::operator=(const std::string & s) {
		Headers.clear();
		SetHeader(s);
		return *this;
	}

	std::vector<unsigned char> HttpRequest::GetMultiFormData(const MultipartFromPart& part) {
		std::vector<unsigned char> ret(part.Size);
		memcpy(&ret[0], &RawContent[part.Start], part.Size);
		return ret;
	}
	WebSocket HttpRequest::GetWebSocket() {
		WebSocket ret = Socket;
		ret.Server = Server;
		return ret;
	}
	std::vector<HttpRequest::RequestRange> HttpRequest::GetRanges() {
		auto ranges = HasHeader("range");
		if (ranges) {
			auto Ranges = ranges->substr(ranges->find(L'=') + 1);
			return PVX::Map(PVX::String::Split_No_Empties(Ranges, L","), [](const std::wstring & part) {
				auto r = PVX::String::Split_No_Empties_Trimed(part, L"-");
				return HttpRequest::RequestRange{ (size_t)_wtoi64(r[0].c_str()), r.size() > 1 ? (size_t)_wtoi64(r[1].c_str()) : -2 };
			});
		}
		return std::vector<HttpRequest::RequestRange>();
	}
	int HttpRequest::SetHeader(const std::string & s) {
		using namespace PVX;
		using namespace PVX::String;
		static std::regex HttpHeaderRegex1("(\\S*)\\s+(\\S+)\\s+(HTTP|HTTPS)/(\\S+)\r\n", std::regex_constants::optimize | std::regex_constants::ECMAScript);
		static std::regex HttpHeaderRegex2("([^\\s:]+)\\s*:\\s*([^\r\n]+)\r\n", std::regex_constants::optimize | std::regex_constants::ECMAScript);

		std::smatch matches;
		if (std::regex_search(s, matches, HttpHeaderRegex1) && matches.size() == 5) {
			Method = matches[1].str();
			QueryString = PVX::Encode::ToString(matches[2].str());
			Protocol = matches[3].str();
			Version = matches[4].str();

			forEach(Filter(PVX::regex_matches(s, HttpHeaderRegex2), [](const auto &m) {return m.size() == 3; }), [&](const auto &m) {
				Headers[ToLower(m[1].str())] = m[2].str();
			});

			auto cookies = Headers.find("cookie");
			if (cookies != Headers.end()) {
				forEach(Split_No_Empties_Trimed(cookies->second, L";"), [&](const std::wstring & c) {
					auto f = c.find('=');
					Cookies[Trim(c.substr(0, f))] = Trim(c.substr(f + 1));
				});
			}
			return 1;
		}
		return 0;
	}
	std::wstring * HttpRequest::HasHeader(const std::string & h) {
		auto f = Headers.find(h);
		if (f != Headers.end())
			return &f->second();
		return nullptr;
	}

	PVX::JSON::Item HttpRequest::Json() const {
		return PVX::JSON::parse(RawContent);
	}

	static std::wregex SplitHeaderRegex(LR"raw(([^\:]+):\s*([^;\r\n]*)(;\s*(.+))?(:?\r\n)?)raw", std::regex_constants::optimize);
	static std::wregex StringsRemoverRegex(LR"regex(\"(([^\"]|\\\")*)\")regex", std::regex_constants::optimize);
	static std::wregex ContentOptionsRegex(LR"raw(\s*([^=]+)\s*=\s*([^;]+);?)raw", std::regex_constants::optimize | std::regex_constants::icase);
	static std::wregex ContentTypeRegex(LR"raw(([^;]+)(;(.*))?)raw", std::regex_constants::optimize|std::regex_constants::icase);

	void GetQueryVariables(const std::wstring & qs, std::map<std::wstring, UtfHelper> & Vars) {
		static std::wregex QueryVarRegex(LR"raw(([^=]+)(=([^&]+))?&?)raw", std::regex_constants::optimize | std::regex_constants::ECMAScript);
		std::wsmatch Match;
		auto Start = qs.cbegin();
		while (std::regex_search(Start, qs.cend(), Match, QueryVarRegex)) {
			if (Match.size() > 3 && Match[1].matched&&Match[3].matched) {
				Vars[Match[1].str()] = PVX::Decode::Uri(Match[3].str());
			} else if (Match.size() > 1 && Match[1].matched) {
				Vars[Match[1]] = L"true";
			}
			Start = Match[0].second;
		}
	}
	void HttpServer::HandleRequest(TcpSocket &Socket, HttpRequest & http, Route & route) {
		if (http.Get->size()) GetQueryVariables(http.Get, http.Variables);
		http.Socket = Socket;
		http.Server = this;
		HttpResponse Response;
		SetDefaultHeader(Response);
		Response.Socket = Socket;

		if (auto h = http.HasHeader("content-type"); h) {
			std::map<std::wstring, std::wstring> ContentOptions;
			auto m = PVX::regex_match(*h, ContentTypeRegex);
			std::wstring ContentType = m[1];
			if (m.size() > 3) PVX::onMatch(m[3], ContentOptionsRegex, [&ContentOptions](const std::wsmatch & mm) { ContentOptions[mm[1]] = mm[2]; });
			if (ContentType == L"multipart/form-data") {
				http.SetMultipartForm(ContentOptions[L"boundary"]);
			}
		}

		try {
			for (auto & f : Filters) {
				if (!f(http, Response)) {
					SendResponse(Socket, http, Response);
					return;
				}
			}

			route.Run(http, Response);
			if (!Response.Handled)
				SendResponse(Socket, http, Response);
		} catch (std::exception& e) {
			Response.Html(e.what());
			Response.StatusCode = 500;
			SendResponse(Socket, http, Response);
		} catch (...) {
			Response.StatusCode = 500;
			SendResponse(Socket, http, Response);
		}
	}
	void HttpRequest::SetMultipartForm(const std::wstring & bound) {
		auto b = PVX::Encode::UTF(L"--" + bound);
		auto bsz = b.size();

		long long Start = 0, End = 0;
		const unsigned char * dt = RawContent.data();
		auto sz = RawContent.size();
		while (End < sz) {
			Start += bsz + 2;
			End = PVX::IndexOfBinary(dt, sz, (const unsigned char*)"\r\n\r\n", 4, Start);
			if (End < 0)return;
			std::wstring cHeaders = PVX::Decode::UTF(dt + Start, End - Start + 4);
			Start = End + 4;
			End = PVX::IndexOfBinary(dt, sz, (const unsigned char*)b.data(), bsz, Start);

			MultipartFromPart part;
			part.Start = Start;
			part.Size = End - Start - 2;

			PVX::onMatch(cHeaders, SplitHeaderRegex, [&part](const std::wsmatch & m) {
				std::wstring Name = m[2];
				std::map<std::wstring, std::wstring> Values;
				std::vector<std::wstring> Strings = PVX::regex_matches<std::wstring>(m[4], StringsRemoverRegex, [](const std::wsmatch & m) { return PVX::Decode::Unescape(m[1].str()); });
				auto tmp = std::regex_replace(m[4].str(), StringsRemoverRegex, L"\"");
				int StringIndex = 0;
				PVX::onMatch(m[4], ContentOptionsRegex, [&Values, &StringIndex, &Strings](const std::wsmatch& m2) {
					std::wstring name = PVX::String::ToLower(m2[1]);
					if (m2[2].str()[0] == '"')
						Values[name] = Strings[StringIndex++];
					else
						Values[name] = m2[2];
				});
				part.Options[PVX::String::ToLower(m[1].str())] = { Name, Values };
			});
			Multipart.push_back(part);
			Start = PVX::IndexOfBinary(dt, sz, (const unsigned char*)b.data(), bsz, End);
		}
	}
	bool HttpRequest::SouldCompress() {
		std::wstring * h = 0;
		return (h = HasHeader("accept-encoding")) && (h->find(L"deflate") != std::wstring::npos);
	}
	std::wstring HttpRequest::operator[](const std::wstring& Name) {
		return Variables[Name]();
	}
	long long HttpRequest::operator()(const std::wstring& Name) {
		return _wtoi64(Variables[Name]().c_str());
	}

	std::map<std::wstring, std::wstring> HttpRequest::GetVariableMap() const {
		std::map<std::wstring, std::wstring> ret;
		for (auto& [n, v] : Variables) ret[n] = v;
		return ret;
	}
	void HttpRequest::BasicAuthentication(std::function<void(const std::wstring& Username, const std::wstring& Password)> clb) {
		const std::wregex r(LR"regex(basic\s+(.+))regex", std::regex_constants::optimize|std::regex_constants::icase);
		if (auto& Auth = *HasHeader("authorization"); &Auth) {
			if (auto match = PVX::regex_match(Auth, r); match.size()) {
				auto [Username, Password] = PVX::String::Split2(PVX::Decode::UTF(PVX::Decode::Base64(PVX::Encode::ToString(match[1]))), L":");
				clb(Username, Password);
			}
		}
	}
}