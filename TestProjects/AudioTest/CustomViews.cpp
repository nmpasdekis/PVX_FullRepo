#include <PVX_Network.h>
#include <PVX_File.h>

using namespace PVX;
using namespace PVX::Network;

std::unordered_map<std::string, std::vector<uint8_t>> CustomViewCache;

PVX::JSON::Item GetViewList() {
	auto files = PVX::IO::Dir("www/customViews");
	PVX::JSON::Item ret = PVX::JSON::jsElementType::Object;
	for (auto& fn : files) {
		auto fn2 = fn.substr(0, fn.size()-5);
		auto txt = PVX::IO::ReadBinary(("www/customViews/" + fn).c_str());
		auto it = PVX::JSON::parse(txt);
		ret[fn2] = it["Category"];
		CustomViewCache[fn2] = txt;
	}
	return ret;
}

void CustomViews(PVX::Network::HttpServer& http) {
	http.Routes(L"/api/view/GetCustomViewList", [](HttpRequest& req, HttpResponse& resp) {
		resp.Json(GetViewList());
	});
	http.Routes(L"/api/view/GetCustomView", [](HttpRequest& req, HttpResponse& resp) {
		resp.StatusCode = 404;
		req.Json().If(L"Name", [&](const JSON::Item& item) {
			auto name = PVX::Encode::UtfString(item.String());
			if (!CustomViewCache.count(name)) {
				auto dt = PVX::IO::ReadBinary(("www/customViews/" + name + ".json").c_str());
				if (dt.size()) {
					CustomViewCache[name] = dt;
					resp.Data(dt, L"application/json");
					resp.StatusCode = 200;
				}
				return;
			}
			resp.Data(CustomViewCache[name], L"application/json");
			resp.StatusCode = 200;
		});
	});
	http.Routes(L"/api/view/SaveCustomView", [](HttpRequest& req, HttpResponse& resp) {
		auto data = req.Json();
		auto Name = PVX::Encode::UtfString(data[L"Name"].String());
		auto View = data[L"View"];
		View["Name"] = Name;
		PVX::IO::Write("www/customViews/" + Name + ".json", View);
	});
}