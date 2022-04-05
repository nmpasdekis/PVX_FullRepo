#include <PVX_Network.h>
#include <PVX_Encode.h>
#include <string>
#include <sstream>
#include <PVX_String.h>
#include <PVX.inl>
#include <zlib.h>
#include <PVX_Deflate.h>
#include <PVX_Regex.h>

namespace PVX {
	namespace Network {
		HttpClient& HttpClient::OnConnect(std::function<void(TcpSocket&)> clb) {
			onConnect = clb;
			return *this;
		}
		HttpClient::HttpClient() : headers{
			{"accept", L"*/*" },
			{"user-agent", L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.77 Safari/537.36" },
			{"accept-encoding", L"deflate" }
		} {}



		int Chuncked(TcpSocket & Socket, std::vector<unsigned char> & data) {
			int rez = -1;
			if (!data.size()) {
				while ((rez = Socket.Receive(data)) >= 0);
				if (rez < 0) {
					data.clear();
					return 3;
				}
			}
			std::vector<unsigned char> ret;
			for (;;) {
				unsigned int ChunckSize = 0;
				int i;
				for (i = 1; i < data.size(); i++) {
					int c = i - 1;
					if (data[c] >= '0' &&  data[c] <= '9')
						ChunckSize = (ChunckSize << 4) | (data[c] - '0');
					else if (data[c] >= 'a' && data[c] <= 'f')
						ChunckSize = (ChunckSize << 4) | (data[c] - 'a' + 10);
					else if (data[c] >= 'A' && data[c] <= 'F')
						ChunckSize = (ChunckSize << 4) | (data[c] - 'A' + 10);
					else if (data[c] == '\r' && data[i] == '\n') {
						break;
					} else
						return 1;
				}
				if (!ChunckSize) {
					data = ret;
					return 0;
				}
				int h = i + 1;
				while (data.size() < (h + ChunckSize + 2) && (rez=Socket.Receive(data)) > 0);
				if (rez < 0 || data[h + ChunckSize]!='\r' || data[h + ChunckSize +1] != '\n') {
					data.clear();
					return 2;
				}
				auto lsz = ret.size();
				ret.resize(lsz + ChunckSize);
				memcpy(&ret[lsz], &data[h], ChunckSize);
				h += ChunckSize + 2;
				for (i = h; i < data.size(); i++) {
					data[i - h] = data[i];
				}
				data.resize(data.size() - h);
			}
		}
		int Chuncked(TcpSocket & Socket, std::vector<unsigned char> & data, std::function<void(const std::vector<unsigned char>&)> onReceiveData) {
			int rez;
			if (!data.size()) {
				while ((rez = Socket.Receive(data)) >= 0);
				if (rez < 0) {
					data.clear();
					return 3;
				}
			}
			onReceiveData(data);
			std::vector<unsigned char> ret;
			std::vector<unsigned char> tmpData;
			for (;;) {
				unsigned int ChunckSize = 0;
				int i;
				for (i = 1; i < data.size(); i++) {
					int c = i - 1;
					if (data[c] >= '0' &&  data[c] <= '9')
						ChunckSize = (ChunckSize << 4) | (data[c] - '0');
					else if (data[c] >= 'a' && data[c] <= 'f')
						ChunckSize = (ChunckSize << 4) | (data[c] - 'a' + 10);
					else if (data[c] >= 'A' && data[c] <= 'F')
						ChunckSize = (ChunckSize << 4) | (data[c] - 'A' + 10);
					else if (data[c] == '\r' && data[i] == '\n') {
						break;
					} else
						return 1;
				}
				if (!ChunckSize) {
					data = ret;
					return 0;
				}
				int h = i + 1;
				while (data.size() < (h + ChunckSize + 2) && (rez = Socket.Receive(tmpData)) > 0) {
					onReceiveData(tmpData);
					auto tsz = data.size();
					data.resize(tsz + tmpData.size());
					memcpy(&data[tsz], tmpData.data(), tmpData.size());
					tmpData.clear();
				}
				if (rez < 0 || data[h + ChunckSize] != '\r' || data[h + ChunckSize + 1] != '\n') {
					data.clear();
					return 2;
				}
				auto lsz = ret.size();
				ret.resize(lsz + ChunckSize);
				memcpy(&ret[lsz], &data[h], ChunckSize);
				h += ChunckSize + 2;
				for (i = h; i < data.size(); i++) {
					data[i - h] = data[i];
				}
				data.resize(data.size() - h);
			}
		}

		HttpClient::HttpResponse HttpClient::Get() {
			HttpClient::HttpResponse ret;
			TcpSocket Socket;
			Socket.SetOption(TcpSocketOption::NoDelay, (int)1);
			auto Header = MakeHeader("GET");

			if (Socket.Connect(domain.c_str(), port.c_str())) {
				ret.StatusCode = 404;
				return ret;
			}
			if (onConnect) onConnect(Socket);

			if (Socket.Send(PVX::Encode::UTF(Header))) {
				Receive(Socket, ret.Headers, ret.Data, ret.Protocol, ret.StatusCode);
			}
			return ret;
		}
		HttpClient::HttpResponse HttpClient::Post(const std::wstring & Data) {
			return Post(PVX::Encode::UTF(Data));
		}

		JSON::Item HttpClient::HttpResponse::Json() {
			return JSON::parse(Data);
		}
		std::vector<unsigned char> HttpClient::HttpResponse::Raw() {
			return Data;
		}
		std::string HttpClient::HttpResponse::Text() {
			std::string ret;
			ret.resize(Data.size());
			memcpy(&ret[0], &Data[0], Data.size());
			return ret;
		}
		std::wstring HttpClient::HttpResponse::UtfText() {
			return PVX::Decode::UTF(Data);
		}

		static bool NeedUriEncode(const std::wstring& str) {
			size_t i;
			auto sz = str.size();
			for (i = 0; i<sz && str[i]<128 && str[i]>32; i++);
			return i < sz;
		}
		static bool NeedUriEncode(const std::string& str) {
			size_t i;
			auto sz = str.size();
			for (i = 0; i<sz && str[i]<128 && str[i]>32; i++);
			return i < sz;
		}

		HttpClient::HttpResponse HttpClient::Post(const std::vector<unsigned char> & Data) {
			HttpClient::HttpResponse ret;
			TcpSocket Socket;
			if (Data.size()) {
				headers["content-length"] = std::to_wstring(Data.size());
			}
			auto Header = MakeHeader("POST");

			if (Socket.Connect(domain.c_str(), port.c_str())) {
				ret.StatusCode = 404;
				return ret;
			}
			if (onConnect) onConnect(Socket);

			if (Socket.Send(PVX::Encode::UTF(Header))) {
				if (Data.size()) if (!Socket.Send(Data)) return ret;
				Receive(Socket, ret.Headers, ret.Data, ret.Protocol, ret.StatusCode);
			}
			return ret;
		}
		HttpClient::HttpResponse HttpClient::Post(const JSON::Item & Data) {
			headers["content-type"] = L"application/json";
			return Post(JSON::stringify(Data));
		}

		HttpClient::HttpResponse HttpClient::Get(const std::string& url) {
			Url(url);
			return Get();
		}
		HttpClient::HttpResponse HttpClient::Post(const std::string& url, const std::vector<unsigned char>& Data) {
			Url(url);
			return Get();
		}
		HttpClient::HttpResponse HttpClient::Post(const std::string& url, const std::wstring& Data) {
			Url(url);
			return Post(Data);
		}
		HttpClient::HttpResponse HttpClient::Post(const std::string& url, const JSON::Item& Data) {
			Url(url);
			return Post(Data);
		}

		std::string ComponentUri(const std::wstring& url) {
			using namespace PVX::Encode;
			if (NeedUriEncode(url)) {
				std::wstring path = url;
				auto qPos = url.find(L'?');
				if (qPos != std::wstring::npos) {
					return PVX::String::Join(PVX::Map(PVX::String::Split(url.substr(0, qPos), L"/"), [](const std::wstring& s) { return Uri(s); }), "/") + 
						"?" +	
						PVX::String::Join(PVX::Map(PVX::String::Split(url.substr(qPos + 1), L"&"), [](const std::wstring& s) {
							return PVX::String::Join(PVX::Map(PVX::String::Split(s, L"="), [](const std::wstring& s) { return Uri(s); }), "=");
						}), "&");
				}
				return PVX::String::Join(PVX::Map(PVX::String::Split(url, L"/"), [](const std::wstring& s) { return Uri(s); }), "/");
			}
			return ToString(url);
		}

		HttpClient::HttpResponse HttpClient::Get(const std::wstring& url) {
			//query = ComponentUri(url);
			Url(url);
			return Get();
		}
		HttpClient::HttpResponse HttpClient::Post(const std::wstring& url, const std::vector<unsigned char>& Data) {
			//query = ComponentUri(url);
			Url(url);
			return Post(Data);
		}
		HttpClient::HttpResponse HttpClient::Post(const std::wstring& url, const std::wstring& Data) {
			//query = ComponentUri(url);
			Url(url);
			return Post(Data);
		}
		HttpClient::HttpResponse HttpClient::Post(const std::wstring& url, const JSON::Item& Data) {
			//query = ComponentUri(url);
			Url(url);
			return Post(Data);
		}


		HttpClient::HttpClient(const std::string& url): HttpClient() {
			urlHelper(PVX::Encode::UriEncode(url));
		}
		HttpClient::HttpClient(const std::wstring& url): HttpClient() {
			urlHelper(PVX::Encode::UriEncode(PVX::Encode::UTF(url)));
		}


		std::string_view HttpClient::DomainHelper(std::string_view src) {
			auto protoEnd = src.find("://");
			if (protoEnd != std::wstring::npos) {
				protocol = PVX::String::ToLower(std::string(src.substr(0, protoEnd)));
				if (protocol=="https")
					port = "443";
				src = src.substr(protoEnd + 3);
			}
			auto domainEnd = src.find("/");
			if (domainEnd == std::wstring::npos) domainEnd = src.size();

			if (domainEnd != 0) {
				auto tmpDomain = src.substr(0, domainEnd);
				auto portIndex = tmpDomain.find(':');
				if (portIndex != std::wstring::npos) {
					domain = tmpDomain.substr(0, portIndex);
					port = tmpDomain.substr(portIndex+1);
				} else {
					domain = tmpDomain;
				}
				src = src.substr(domainEnd);
			}
			return src;
		}

		void HttpClient::urlHelper(const std::string_view& src) {
			path = "/";
			path += (src.size() && src[0]=='/') ? src.substr(1) : src;
		}

		void HttpClient::Url(const std::string& Src) {
			urlHelper(PVX::Encode::UriEncode(Src));
		}
		void HttpClient::Url(const std::wstring& Src) {
			urlHelper(PVX::Encode::UriEncode(PVX::Encode::UTF(Src)));
		}
		
		HttpClient & HttpClient::OnReceiveHeader(std::function<void(const std::wstring&)> fnc) {
			onReceiveHeader = fnc;
			return *this;
		}

		HttpClient & HttpClient::OnReceiveData(std::function<void(const std::vector<unsigned char>&)> fnc) {
			onReceiveData = fnc;
			return *this;
		}

		HttpClient& HttpClient::Headers(const std::unordered_map<std::string, std::wstring>& H) {
			for (auto& [Name, Value]  : H) {
				headers[PVX::String::ToLower(Name)] = Value;
			}
			return *this;
		}

		HttpClient& HttpClient::Headers_Raw(const std::unordered_map<std::string, std::wstring>& H) {
			for (auto& [Name, Value] : H) {
				headers[Name] = Value;
			}
			return *this;
		}

		HttpClient& HttpClient::HeadersAll(const std::unordered_map<std::string, std::wstring>& H) {
			headers = std::unordered_map<std::string, PVX::Network::UtfHelper>();
			for (auto& [Name, Value] : H) {
				headers[PVX::String::ToLower(Name)] = Value;
			}
			return *this;
		}

		HttpClient& HttpClient::HeadersAll_Raw(const std::unordered_map<std::string, std::wstring>& H) {
			headers = std::unordered_map<std::string, PVX::Network::UtfHelper>();
			for (auto& [Name, Value] : H) {
				headers[Name] = Value;
			}
			return *this;
		}

		UtfHelper& HttpClient::operator[](const std::string& Name) {
			return headers[PVX::String::ToLower(Name)];
		}

		std::wstring HttpClient::MakeHeader(const char * Verb) {
			using namespace PVX::Encode;
			std::wstringstream ret;
			{
				std::stringstream tmp;
				tmp << Verb << " " << path << " HTTP/1.1\r\n" <<
					"Host: " << domain << "\r\n";
				ret << ToString(tmp.str());
			}
			for (auto & h : headers) {
				ret << ToString(h.first) << L": " << h.second() << L"\r\n";
			}
			ret << L"\r\n";
			return ret.str();
		}

		std::wstring ToLower(const std::wstring & s) {
			std::wstring ret;
			ret.resize(s.size());
			std::transform(s.begin(), s.end(), ret.begin(), [](wchar_t c) { return c | ('a'^'A'); });
			return ret;
		}
		void ToLowerInplace(std::wstring & s) {
			std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return c | ('a'^'A'); });
		}

		int HttpClient::Receive(PVX::Network::TcpSocket & Socket, std::vector<std::pair<std::wstring, std::wstring>> & Headers, std::vector<unsigned char> & Data, std::wstring & Proto, int & Status) {
			using namespace PVX;
			using namespace PVX::String;
			std::wstring Header;
			while (Socket.Receive(Data) > 0) {
				for (auto i = 3; i < Data.size(); i++) {
					if (Data[i - 3] == '\r' && Data[i - 2] == '\n' && Data[i - 1] == '\r' && Data[i] == '\n') {
						i++;
						Header = PVX::Decode::UTF(Data.data(), i);
						for (int j = i; j < Data.size(); j++) {
							Data[j - i] = Data[j];
						}
						Data.resize(Data.size() - i);
						break;
					}
				}
				if (Header.size()) break;
			}
			if (!Header.size())return 2;
			{
				if (onReceiveHeader != nullptr) onReceiveHeader(Header);
				auto Lines = Split_No_Empties_Trimed(Header, L"\r\n");
				Proto = Lines[0].substr(0, Lines[0].find(L'/'));
				Status = std::stoi(Lines[0].substr(Lines[0].find(L' ')));

				Headers.resize(Lines.size() - 1);

				std::transform(Lines.begin() + 1, Lines.end(), Headers.begin(), [](const std::wstring & line) {
					auto ar = Split_No_Empties_Trimed(line, L":");
					ToLowerInplace(ar[0]);
					return std::make_pair(ar[0], ar[1]);
				});
			}
			uint64_t ContentLenght = 0;
			int IsChunked = 0;
			int IsDeflated = 0;

			for (auto & [Name, Value] : Headers) {
				if (Name == L"transfer-encoding" || Name == L"content-encoding") {
					auto h2 = ToLower(Value);
					if (h2.find(L"deflate") != std::wstring::npos) 
						IsDeflated = 1;
					if (h2.find(L"chunked") != std::wstring::npos) 
						IsChunked = 1;
				} else if (Name == L"content-length") {
					ContentLenght = std::stoll(Value);
				} else if (Name == L"set-cookie") {
					auto c = Split_No_Empties_Trimed(Value, L";");
					auto cookie = Split_Trimed(c[0], L"=");
					Cookies[cookie[0]] = cookie[1];
				}
			}
			int rez = 0;
			if (IsChunked) {
				if(onReceiveData!=nullptr)
					rez = Chuncked(Socket, Data, onReceiveData);
				else
					rez = Chuncked(Socket, Data);
			} else {
				if (onReceiveData != nullptr) {
					if (Data.size()) onReceiveData(Data);
					std::vector<unsigned char> tmpData;
					while (Data.size() < ContentLenght && (rez = Socket.Receive(tmpData)) > 0) {
						onReceiveData(tmpData);
						auto sz = Data.size();
						Data.resize(sz + tmpData.size());
						memcpy(&Data[sz], tmpData.data(), tmpData.size());
						tmpData.clear();
					}
				} else {
					while (Data.size() < ContentLenght && (rez = Socket.Receive(Data)) > 0);
				}
			}
			if (!Data.size()) return 1;

			if (IsDeflated) {
				Data = PVX::Compress::Inflate(Data);
			}

			return 0;
		}

		HttpClient::HttpResponse::HttpResponse() {

		}
	}
}