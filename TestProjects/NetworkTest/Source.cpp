#include <PVX_Network.h>

int main() {
	using namespace PVX::Network;

	HttpServer http;

	http.Routes(L"/api/test", [](HttpRequest& req, HttpResponse& resp) {
		resp.Json({
			{ L"Message", L"Hello" },
			{ L"Message2", L"Hello" },
		});
	});
	http.ContentRoute(L"/js", L"html\\js");
	http.DefaultHtml(L"html\\index.html");


	TcpServer serv;
	serv.Serve(http);
	getchar();
	return 0;
}