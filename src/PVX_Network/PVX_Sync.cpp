#include <PVX_Network.h>
#include <PVX_String.h>

namespace PVX::Network {

	WebSocketServer& CreateSyncServer(HttpServer& http, std::wstring url, SyncServer* server) {
		auto & ws = http.CreateWebSocketServer(url, [server](WebSocketServer& ws, const std::string& connectionId, const std::vector<unsigned char>& Data) {
			auto& Server = *server;
			if (Data[0] == 'R') {
				auto run = PVX::JSON::parse(Data.data() + 1, Data.size() - 1);
				auto Function = run["function"];
				auto Params = run["params"];
				auto target = run["target"].String();
				if (target == L"all") {
					ws.SendAll([&](WebSocketPacket& pk) {
						pk.Json({
							{ L"fnc", Function },
							{ L"params", Params }
						});
					});
				}
				else if (target == L"others") {
					ws.SendAllExceptOne(connectionId, [&](WebSocketPacket& pk) {
						pk.Json({
							{ L"fnc", Function },
							{ L"params", Params }
						});
					});
				}
				else if (target == L"server") {
					auto fnc = Function.String();
					if (Server.Functions.count(fnc))
						Server.Functions[fnc](ws, connectionId, Params);
				}
			}
		});
		return ws;
	}

	SyncServer::SyncServer(HttpServer& http, std::wstring url): ws{ CreateSyncServer(http, url, this)} {}
	void SyncServer::OnConnect(std::function<void(const std::string&, WebSocket&)> fnc) {
		ws.OnConnect(fnc);
	}
	void SyncServer::OnDisconnect(std::function<void(const std::string&)> fnc) {
		ws.OnDisconnect(fnc);
	}
	void SyncServer::AddFunction(const std::wstring& Name, srvFunction ServerFunction) {
		Functions[Name] = ServerFunction;
	}
}