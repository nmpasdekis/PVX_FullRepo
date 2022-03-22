#include <PVX_Network.h>
#include <PVX_File.h>

using namespace PVX;
using namespace PVX::Network;

std::unordered_map<std::wstring, std::vector<uint8_t>> CustomViewCache;

PVX::JSON::Item GetViewList() {
	auto files = PVX::IO::Dir(L"www\\customViews\\*.json");
	PVX::JSON::Item ret = PVX::JSON::EmptyObject();
	for (auto& fn : files) {
		auto it = PVX::IO::LoadJson((L"www\\customViews\\" + fn).c_str());
		ret[fn.substr(0, fn.size()-5)] = it["Category"];
		CustomViewCache[fn.substr(0, fn.size()-5)] = PVX::Encode::UTF(JSON::stringify(it));
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
			auto name = item.String();
			if (!CustomViewCache.count(name)) {
				auto dt = PVX::IO::ReadBinary((L"www\\customViews\\" + item.String() + L".json").c_str());
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
		auto Name = data[L"Name"].String();
		auto View = data[L"View"];
		View["Name"] = Name;
		PVX::IO::Write(L"www\\customViews\\" + Name + L".json", View);
	});
}