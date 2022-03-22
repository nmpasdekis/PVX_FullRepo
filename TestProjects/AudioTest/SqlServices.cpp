#include <PVX_Network.h>
#include <PVX_File.h>
#include <PVX.inl>
#include <PVX_MSSQL.h>
#include <PVX_Encrypt.h>

using namespace PVX;
using namespace PVX::Network;
using namespace PVX::Encrypt;


constexpr auto VerifyPassword = IdentityPasswordVerifier<SHA1_Algorithm, 128, 256, 1000>;

void SqlServices(PVX::Network::HttpServer& http) {

	http.Routes(L"/account/login", [&](HttpRequest& req, HttpResponse& resp) {
		using namespace std::string_literals;
		auto Login = req.Json();
		std::wstring Username = Login[L"username"].String();

		//auto userRes = db.JsonQueryJson(L"select PasswordHash, dbo.UserRoles(Id) parseJSON_Roles from AspNetUsers where Username=@username", { { L"username", Username } });

		auto userRes = PVX::IO::LoadJson((L"www\\db\\users\\"s + Username + L".json"s).c_str());

		if (userRes.length()) {
			if (VerifyPassword(Login[L"password"].String(), userRes[0][L"PasswordHash"])) {
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
		resp.ServeFile(L"www\\db\\roles.json");
	});
}