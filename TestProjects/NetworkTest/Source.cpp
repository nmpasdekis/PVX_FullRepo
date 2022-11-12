#define _CRT_SECURE_NO_WARNINGS
#include <PVX_Network.h>
#include <PVX_ComPort.h>

#include <PVX_String.h>

#include <filesystem>
#include <iostream>
#include <PVX_File.h>

#include <stdlib.h>


int main() {
	using namespace PVX::Network;
	using namespace PVX;
	using namespace PVX::JSON;

	HttpServer http;
	PVX::Serial::Com comm9(9);

	uint8_t lastMode = 0;
	uint8_t lastFan = 0;
	uint8_t lastTemp = 25;

	http.AddFilter([](HttpRequest& req, HttpResponse& resp) {
		return 1;
	});

	http.Routes(L"/api/ac/state", [&](HttpResponse& resp) {
		resp.Json({  
			{ L"mode", lastMode },
			{ L"fan", lastFan },
			{ L"temp", lastTemp },
		});
	});

	http.Routes(L"/api/ac/command", [&](HttpRequest& req, HttpResponse& resp) {
		auto body = req.Json();
		uint8_t on = body[L"on"].Integer();
		uint8_t Mode = (uint8_t)body[L"mode"].Integer();
		uint8_t Fan = (uint8_t)body[L"fan"].Integer();
		uint8_t Temp = (uint8_t)body[L"temp"].Integer();
		if (on==1) {
			lastMode = Mode;
			lastFan = Fan;
			lastTemp = Temp;
		}

		uint8_t cmd[]{ on, Mode, Fan, Temp };
		comm9.Write(cmd, 4);		
	});
	http.ContentRoute(L"/js", L"html\\js");
	http.DefaultHtml(L"html\\index.html");


	TcpServer serv;
	serv.Serve(http);
	getchar();
	return 0;
}