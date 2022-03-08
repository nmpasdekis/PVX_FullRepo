#include <PVX_Network.h>

int main() {
	using namespace PVX::Network;

	HttpServer http;

	http.Routes(L"/api/test", [](HttpRequest& req, HttpResponse& resp) {
		resp.Json({
			{ L"Message", L"Hello" }
		});
	});



	TcpServer serv("80");
	serv.Serve(http.GetHandler());
	return 0;
}