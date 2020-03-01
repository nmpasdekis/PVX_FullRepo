#include <PVX_Network.h>
#include <PVX_Encode.h>
#include <PVX_Encrypt.h>
#include <PVX_File.h>
#include <sstream>

namespace PVX::Network {
	HttpResponse::HttpResponse() {
		SouldCompress = true;
		Handled = false;
		StatusCode = 200;
	}

	UtfHelper & HttpResponse::operator[](const std::wstring & Name) {
		std::wstring name = Name;
		for (auto & c : name) if (c >= 'A'&&c <= 'Z') { c += 'a' - 'A'; }
		return Headers[name];
	}

	void HttpResponse::Json(const PVX::JSON::Item & json) {
		Content.WriteUTF(PVX::JSON::stringify(json));
		Headers[L"content-type"] = L"application/json";
	}

	void HttpResponse::UtfData(const std::wstring & data, const std::wstring & contentType) {
		Content.WriteUTF(data);
		Headers[L"content-type"] = contentType;
	}
	void HttpResponse::Serve(const std::vector<unsigned char> & data, const std::wstring & contentType) {
		Content.SetData(data);
		Headers[L"content-type"] = contentType;
	}

	void HttpResponse::BeginRange(size_t Size) {
		RangeSize = Size;
		StatusCode = 206;
		Headers[L"content-type"] = L"multipart/byteranges; boundary=pvx_sep";
	}

	void HttpResponse::AddRange(const std::wstring & ContentType, size_t Offset, const std::vector<unsigned char>& Data) {
		typedef const unsigned char* raw;
		Content << (raw)"--pvx_sep\r\ncontent-type: " <<
			ContentType << (raw)"\r\ncontent-range: bytes " <<
			Offset << (raw)"-" << (Offset + Data.size() - 1) << (raw)"/" << RangeSize << (raw)"\r\n\r\n" << Data;
	}

	void HttpResponse::EndRange() {
		typedef const unsigned char* raw;
		Content << (raw)"--pvx_sep--";
	}

	int HttpResponse::StreamFile(HttpRequest& req, const std::wstring& Filename, int BufferSize) {
		auto r = req.GetRanges();
		if(r.size())
			return SingleRangeFile(r[0].Start, r[0].End - r[0].Start, Filename, BufferSize);
		return SingleRangeFile(0, -1, Filename, BufferSize);
	}

	int HttpResponse::StreamFile(const std::wstring & Filename, int BufferSize, const std::wstring & Mime) {
		using namespace PVX::IO;
		SouldCompress = false;

		FILE * fin;
		Content.GetDataVector().reserve(BufferSize);
		unsigned char * Data = Content.GetData();
		if (_wfopen_s(&fin, Filename.c_str(), L"rb"))return 404;



		Headers[L"content-type"] = PVX::Encode::ToString(Mime.size() ? Mime : Server->GetMime(PVX::IO::FileExtension(Filename)));
		auto fsz = FileSize(fin);

		StreamRaw(fsz, [fsz, fin, Data, BufferSize](TcpSocket & Socket) {
			size_t sz = fread_s(Data, BufferSize, 1, BufferSize, fin);

			if (sz) sz = Socket.Send(Data, sz);
			if (sz < BufferSize) {
				fclose(fin);
				return false;
			}
			return true;
		});
		return 200;
	}

	int HttpResponse::ServeFile(const std::wstring & Filename, const std::wstring & Mime) {
		if (Content.BinaryFile(Filename.c_str())==200) {
			Headers[L"content-type"] = PVX::Encode::ToString(Mime.size() ? Mime : Server->GetMime(PVX::IO::FileExtension(Filename)));
			StatusCode = 200;
			return 200;
		}
		StatusCode = 404;
		return 404;
	}

	int HttpResponse::SingleRangeFile(size_t Offset, size_t Size, const std::wstring & Filename, int FragmentSize) {
		StatusCode = 206;
		Headers[L"content-type"] = PVX::Encode::ToString(Server->GetMime(PVX::IO::FileExtension(Filename)));
		using namespace PVX::IO;
		SouldCompress = false;

		FILE * fin;
		Content.GetDataVector().reserve(FragmentSize);
		unsigned char * Data = Content.GetData();
		if (_wfopen_s(&fin, Filename.c_str(), L"rb")) {
			StatusCode = 404;
			return 1;
		}
		auto fsz = FileSize(fin);
		if (Offset >= fsz) {
			StatusCode = 500;
			return 1;
		}

		if (Size + Offset > fsz) Size = fsz - Offset;

		Headers[L"content-range"] = (std::wstringstream() << L"bytes " << Offset << L"-" << (Size + Offset - 1) << L"/" << fsz).str();
		Headers[L"ETag"] = Filename;

		fseek(fin, Offset, SEEK_SET);
		fsz = Offset + Size;

		StreamRaw(Size, [fsz, fin, Data, FragmentSize](TcpSocket & Socket) {
			auto fs = FragmentSize;
			size_t Offset = ftell(fin);
			if (Offset + fs > fsz) fs = fsz - Offset;


			int sz = fread_s(Data, fs, 1, fs, fin);
			if (sz) sz = Socket.Send(Data, sz);
			if (sz < FragmentSize) {
				fclose(fin);
				return false;
			}
			return true;
		});
		return 0;
	}

	void HttpResponse::StreamRaw(size_t Size, std::function<bool(TcpSocket & Socket)> fnc) {
		Streams.push_back({ Size, fnc });
	}

	void HttpResponse::ClearCookie(const std::wstring & Name) {
		MoreHeaders.push_back(SimpleTuple{ L"set-cookie", Name + L"=; expires=Thu, 01 Jan 1970 00:00:00 GMT" });
	}
	void HttpResponse::SetCookie(const std::wstring & Name, const std::wstring & Value) {
		MoreHeaders.push_back(SimpleTuple{ L"set-cookie", Name + L"=" + Value });
	}

	void HttpResponse::Redirect(const std::wstring & Location, int Status) {
		Headers[L"location"] = Location;
		StatusCode = Status;
	}

	void HttpResponse::Json(const std::wstring & json) {
		Content.WriteUTF(json);
		Headers[L"content-type"] = L"application/json";
	}

	void HttpResponse::Json(const std::vector<unsigned char>& json) {
		Content.SetData(json);
		Headers[L"content-type"] = L"application/json";
	}

	void HttpResponse::Html(const std::wstring & html) {
		Content.WriteUTF(html);
		Headers[L"content-type"] = L"text/html";
	}

	void HttpResponse::Html(const std::string & html) {
		Content.WriteText(html);
		Headers[L"content-type"] = L"text/html";
	}

	void HttpResponse::Data(const std::vector<unsigned char>& data, const std::wstring & contentType) {
		Content.SetData(data.data(), data.size());
		Headers[L"content-type"] = contentType;
	}

	static std::map<const int, std::string> StatusCodes{
		{ 100, "HTTP/1.1 100 Continue\r\n" },
		{ 101, "HTTP/1.1 101 Switching Protocols\r\n" },
		{ 102, "HTTP/1.1 102 Processing (WebDAV; RFC 2518)\r\n" },
		{ 200, "HTTP/1.1 200 OK\r\n" },
		{ 201, "HTTP/1.1 201 Created\r\n" },
		{ 202, "HTTP/1.1 202 Accepted\r\n" },
		{ 203, "HTTP/1.1 203 Non-Authoritative Information (since HTTP/1.1)\r\n" },
		{ 204, "HTTP/1.1 204 No Content\r\n" },
		{ 205, "HTTP/1.1 205 Reset Content\r\n" },
		{ 206, "HTTP/1.1 206 Partial Content\r\n" },
		{ 207, "HTTP/1.1 207 Multi-Status (WebDAV; RFC 4918)\r\n" },
		{ 208, "HTTP/1.1 208 Already Reported (WebDAV; RFC 5842)\r\n" },
		{ 226, "HTTP/1.1 226 IM Used (RFC 3229)\r\n" },
		{ 300, "HTTP/1.1 300 Multiple Choices\r\n" },
		{ 301, "HTTP/1.1 301 Moved Permanently\r\n" },
		{ 302, "HTTP/1.1 302 Found\r\n" },
		{ 303, "HTTP/1.1 303 See Other (since HTTP/1.1)\r\n" },
		{ 304, "HTTP/1.1 304 Not Modified\r\n" },
		{ 305, "HTTP/1.1 305 Use Proxy (since HTTP/1.1)\r\n" },
		{ 306, "HTTP/1.1 306 Switch Proxy\r\n" },
		{ 307, "HTTP/1.1 307 Temporary Redirect (since HTTP/1.1)\r\n" },
		{ 308, "HTTP/1.1 308 Permanent Redirect (Experimental RFC; RFC 7238)\r\n" },
		{ 400, "HTTP/1.1 400 Bad Request\r\n" },
		{ 401, "HTTP/1.1 401 Unauthorized\r\n" },
		{ 402, "HTTP/1.1 402 Payment Required\r\n" },
		{ 403, "HTTP/1.1 403 Forbidden\r\n" },
		{ 404, "HTTP/1.1 404 Not Found\r\n" },
		{ 405, "HTTP/1.1 405 Method Not Allowed\r\n" },
		{ 406, "HTTP/1.1 406 Not Acceptable\r\n" },
		{ 407, "HTTP/1.1 407 Proxy Authentication Required\r\n" },
		{ 408, "HTTP/1.1 408 Request Timeout\r\n" },
		{ 409, "HTTP/1.1 409 Conflict\r\n" },
		{ 410, "HTTP/1.1 410 Gone\r\n" },
		{ 411, "HTTP/1.1 411 Length Required\r\n" },
		{ 412, "HTTP/1.1 412 Precondition Failed\r\n" },
		{ 413, "HTTP/1.1 413 Request Entity Too Large\r\n" },
		{ 414, "HTTP/1.1 414 Request-URI Too Long\r\n" },
		{ 415, "HTTP/1.1 415 Unsupported Media Type\r\n" },
		{ 416, "HTTP/1.1 416 Requested Range Not Satisfiable\r\n" },
		{ 417, "HTTP/1.1 417 Expectation Failed\r\n" },
		{ 418, "HTTP/1.1 418 I'm a teapot (RFC 2324)\r\n" },
		{ 419, "HTTP/1.1 419 Authentication Timeout (not in RFC 2616)\r\n" },
		{ 420, "HTTP/1.1 420 Method Failure (Spring Framework)\r\n" },
		{ 420, "HTTP/1.1 420 Enhance Your Calm (Twitter)\r\n" },
		{ 422, "HTTP/1.1 422 Unprocessable Entity (WebDAV; RFC 4918)\r\n" },
		{ 423, "HTTP/1.1 423 Locked (WebDAV; RFC 4918)\r\n" },
		{ 424, "HTTP/1.1 424 Failed Dependency (WebDAV; RFC 4918)\r\n" },
		{ 426, "HTTP/1.1 426 Upgrade Required\r\n" },
		{ 428, "HTTP/1.1 428 Precondition Required (RFC 6585)\r\n" },
		{ 429, "HTTP/1.1 429 Too Many Requests (RFC 6585)\r\n" },
		{ 431, "HTTP/1.1 431 Request Header Fields Too Large (RFC 6585)\r\n" },
		{ 440, "HTTP/1.1 440 Login Timeout (Microsoft)\r\n" },
		{ 444, "HTTP/1.1 444 No Response (Nginx)\r\n" },
		{ 449, "HTTP/1.1 449 Retry With (Microsoft)\r\n" },
		{ 450, "HTTP/1.1 450 Blocked by Windows Parental Controls (Microsoft)\r\n" },
		{ 451, "HTTP/1.1 451 Unavailable For Legal Reasons (Internet draft)\r\n" },
		{ 451, "HTTP/1.1 451 Redirect (Microsoft)\r\n" },
		{ 494, "HTTP/1.1 494 Request Header Too Large (Nginx)\r\n" },
		{ 495, "HTTP/1.1 495 Cert Error (Nginx)\r\n" },
		{ 496, "HTTP/1.1 496 No Cert (Nginx)\r\n" },
		{ 497, "HTTP/1.1 497 HTTP to HTTPS (Nginx)\r\n" },
		{ 498, "HTTP/1.1 498 Token expired/invalid (Esri)\r\n" },
		{ 499, "HTTP/1.1 499 Client Closed Request (Nginx)\r\n" },
		{ 499, "HTTP/1.1 499 Token required (Esri)\r\n" },
		{ 500, "HTTP/1.1 500 Internal Server Error\r\n" },
		{ 501, "HTTP/1.1 501 Not Implemented\r\n" },
		{ 502, "HTTP/1.1 502 Bad Gateway\r\n" },
		{ 503, "HTTP/1.1 503 Service Unavailable\r\n" },
		{ 504, "HTTP/1.1 504 Gateway Timeout\r\n" },
		{ 505, "HTTP/1.1 505 HTTP Version Not Supported\r\n" },
		{ 506, "HTTP/1.1 506 Variant Also Negotiates (RFC 2295)\r\n" },
		{ 507, "HTTP/1.1 507 Insufficient Storage (WebDAV; RFC 4918)\r\n" },
		{ 508, "HTTP/1.1 508 Loop Detected (WebDAV; RFC 5842)\r\n" },
		{ 509, "HTTP/1.1 509 Bandwidth Limit Exceeded (Apache bw/limited extension)[26]\r\n" },
		{ 510, "HTTP/1.1 510 Not Extended (RFC 2774)\r\n" },
		{ 511, "HTTP/1.1 511 Network Authentication Required (RFC 6585)\r\n" },
		{ 520, "HTTP/1.1 520 Origin Error (CloudFlare)\r\n" },
		{ 521, "HTTP/1.1 521 Web server is down (CloudFlare)\r\n" },
		{ 522, "HTTP/1.1 522 Connection timed out (CloudFlare)\r\n" },
		{ 523, "HTTP/1.1 523 Proxy Declined Request (CloudFlare)\r\n" },
		{ 524, "HTTP/1.1 524 A timeout occurred (CloudFlare)\r\n" },
		{ 598, "HTTP/1.1 598 Network read timeout error (Unknown)\r\n" },
		{ 599, "HTTP/1.1 599 Network connect timeout error (Unknown)\r\n" }
	};
	std::wstring GetDate() {
		time_t t = time(0);
		struct tm dt;
		localtime_s(&dt, &t);
		wchar_t tmp[2048];
		const wchar_t * Day[7] = { L"Sun", L"Mon", L"Tus", L"Wed", L"Thu", L"Fri", L"Sat" };
		const wchar_t * Month[12] = { L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
		swprintf(tmp, 2047, L"%s, %d %s %d %02d:%02d:%02d GMT",
			Day[dt.tm_wday], dt.tm_mday, Month[dt.tm_mon],
			1900 + dt.tm_year, dt.tm_hour, dt.tm_min, dt.tm_sec);
		return tmp;
	}

	int HttpResponse::SendHeader(size_t ContentLength) {
		PVX_DataBuilder Response;
		Response << StatusCodes[StatusCode];
		Headers[L"date"] = GetDate();

		if (ContentLength) {
			wchar_t tmp[128];
			_ui64tow_s(ContentLength, tmp, 128, 10);
			Headers[L"content-length"] = tmp;
		}

		for (auto & h : Headers) Response << h.first << ": " << (std::wstring) h.second << "\r\n";
		for (auto & h : MoreHeaders) Response << h.Name << ": " << h.Value << "\r\n";

		Response << "\r\n";
		return Socket.Send(Response.GetDataVector()) < 0;
	}

	void HttpResponse::AllowOrigin(HttpRequest& req, const std::set<std::wstring>& Allow) {
		std::wstring origin = req.Headers["origin"];
		if (!Allow.size()||Allow.find(origin)!=Allow.end())
			(*this)[L"Access-Control-Allow-Origin"] = origin;
	}

	void HttpResponse::MakeWebToken(const PVX::JSON::Item& User) {
		using namespace PVX::Encrypt;
		auto usr = PVX::Encode::UTF(PVX::JSON::stringify(User));
		usr.resize(3 * ((usr.size() + 32 + 2) / 3) - 32);
		auto hash = HMAC<SHA256_Algorithm>(Server->TokenKey, usr);
		std::vector<unsigned char> FullToken(32 + usr.size());
		memcpy(&FullToken[0], hash.data(), 32);
		memcpy(&FullToken[32], usr.data(), usr.size());
		SetCookie(L"pxx-token", PVX::Encode::ToString(PVX::Encode::Base64Url(FullToken)));
	}

}