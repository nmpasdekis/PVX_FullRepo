#include <PVX_Network.h>
#include <PVX_File.h>
#include <PVX.inl>
#include <PVX_Encrypt.h>

#include <iostream>

using namespace PVX;
using namespace PVX::Network;
using namespace PVX::Encrypt;


constexpr auto VerifyPassword = IdentityPasswordVerifier<SHA1_Algorithm, 128, 256, 1000>;

void Account(PVX::Network::HttpServer& http) {
	http.Routes(L"/account/login", [&](HttpRequest& req, HttpResponse& resp) {
		using namespace std::string_literals;
		auto Login = req.Json();
		std::wstring Username = Login[L"username"].String();
		std::wcout << Username << L"\n";
		
		auto userFile = L"www/db/users/"s + Username + L".json"s;

		std::wcout << userFile << L"\n";
		auto userRes = PVX::IO::LoadJson(userFile.c_str());
		std::wcout << L"OK\n";

		if (userRes.length()) {
			if (VerifyPassword(Login[L"password"].String(), userRes[0][L"PasswordHash"].String())) {
				resp.MakeWebToken({
					{ L"Username", Username },
					{ L"Roles", userRes[0][L"Roles"] }
				});
				if (!Login.If(L"returnUrl", [&](const JSON::Item& retUrl) {
					resp.Redirect(retUrl.String());
				})) {
					resp.Redirect(L"/");
				}
			} else {
				resp.StatusCode = 403;
			}
		} else {
			resp.StatusCode = 404;
		}
	});
	http.Routes(L"/account/logoff", [](HttpResponse& resp) {
		resp.ClearCookie(L"pvx-token", L"/");
	});
	http.Routes(L"/account/roles", [&](HttpRequest& req, HttpResponse& resp) {
		resp.ServeFile(L"www/db/roles.json");
	});
}