#define _CRT_SECURE_NO_WARNINGS
#include <PVX_Network.h>

#include <PVX_String.h>

#include <filesystem>
#include <iostream>
#include <PVX_File.h>

#include <stdlib.h>


int main() {

	auto fp = PVX::IO::FindFileFullPath(L"change.txt");
	fp = PVX::IO::FindFileFullPath(L"D:\\__Αρχοντούλα\\butterfly_low.ai");
	fp = PVX::IO::FindFileFullPath(L"config.json");

	//std::filesystem::create_directories(L"testDir");
	{
		auto cp = PVX::IO::CurrentPath();
		cp.push_back(std::filesystem::path::preferred_separator);
		auto f = std::filesystem::path(cp + "change.txt");
		auto d = std::filesystem::directory_entry{ f };
		auto s = d.status();
		auto path = f.relative_path().wstring();
		path.c_str();
	}
	PVX::IO::ChangeEventer Eventer;

	Eventer.Track(L"change.txt", [] {
		std::cout << "Changed\n";
	});



	using namespace PVX::Network;
	using namespace PVX;
	using namespace PVX::JSON;

	auto txt = PVX::Replace(L"nikos 1 2 3", std::wregex(L"\\d"), [](const std::wstring& m) {
		return L"_";
	});

	HttpServer http;

	PVX::JSON::Item x = L"{\"Message\":\"Hello\",\"Message2\":[\"Hello\",\"Hello2\", 123, 321.555, false, null]}"_json;
	PVX::JSON::Item x2 = true;
	auto x3 = LR"(
{
	"Test4":[1,2,3,"Nikos"],
	"Test3":"Text",
	"Test2":31.500000,
	"Test":123
}
		)"_json;

	auto testJ = x3["Test4"];

	http.Routes(L"/api/test", [](HttpRequest& req, HttpResponse& resp) {
//		resp.Json(LR"(
//{
//	"Message": "nikos",
//	"Data": [
//		1, 0.5, "test", true, false, null
//	]
//}
//		)"_json);
		Item ret = jsArray{ 1, 2, 3 };
		resp.Json({ { L"Test4", ret } });
	});
	http.ContentRoute(L"/js", L"html\\js");
	http.DefaultHtml(L"html\\index.html");


	TcpServer serv;
	serv.Serve(http);
	getchar();
	return 0;
}