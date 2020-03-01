#include <PVX_Network.h>
#include <PVX_Encode.h>
#include <PVX_Encrypt.h>
#include <PVX_String.h>
#include <PVX.inl>
#include <assert.h>

namespace PVX::Network {
	struct WebPacketHeader {
		int IsLast;
		int op;
		unsigned char Mask[4];
		int headerSize;
		size_t size;
		unsigned char * Data;
	};
	void DecodeWebPacketHeader(const unsigned char * data, WebPacketHeader & h) {
		auto start = data;
		h.IsLast = data[0] >> 7;
		h.op = data[0] & 0x7f;
		h.size = data[1] & 0x7f;
		int HasMask = data[1] >> 7;
		data += 2;
		if (h.size == 126) {
			h.size = (data[0] << 8) | data[1];
			data += 2;
		} else if (h.size == 127) {
			unsigned char * psz = (unsigned char *)&h.size;
			psz[0] = data[7];
			psz[1] = data[6];
			psz[2] = data[5];
			psz[3] = data[4];
			psz[4] = data[3];
			psz[5] = data[2];
			psz[6] = data[1];
			psz[7] = data[0];
			data += 8;
		}
		if (HasMask) {
			h.Mask[0] = data[0];
			h.Mask[1] = data[1];
			h.Mask[2] = data[2];
			h.Mask[3] = data[3];
			data += 4;
		}
		h.headerSize = data - start;
		h.Data = (unsigned char*)data;
	}
	int WebSocket::Receive() {
		if (!HasMore)
			Message.clear();
		struct {
			unsigned char Opcode, Size;
		} Header{};

		Header.Opcode &= 0x07;
		unsigned char Bytes[8];
		size_t MessageSize = 0;

		unsigned char * SizeBytes = (unsigned char*)&MessageSize;
		unsigned char Mask[4];

		int sz = Socket.ReceiveAsync(&Header, 2);
		if (sz == 2) {
			Opcode = (WebSocker_Opcode)(Header.Opcode & 0x0f);
			HasMore = !(Header.Opcode & 0x80);
			int HasMask = Header.Size >> 7;
			MessageSize = Header.Size &= 0x7f;
			if (Header.Size == 126) {
				Socket.Receive(Bytes, 2);
				SizeBytes[0] = Bytes[1]; SizeBytes[1] = Bytes[0];
			} else if (Header.Size == 127) {
				Socket.Receive(Bytes, 8);
				SizeBytes[0] = Bytes[7]; SizeBytes[1] = Bytes[6]; SizeBytes[2] = Bytes[5]; SizeBytes[3] = Bytes[4]; SizeBytes[4] = Bytes[3]; SizeBytes[5] = Bytes[2]; SizeBytes[6] = Bytes[1]; SizeBytes[7] = Bytes[0];
			}
			auto oldSize = Message.size();
			Message.resize(oldSize + MessageSize);
			if (HasMask) Socket.Receive(Mask, 4);
			if (MessageSize)sz += Socket.Receive(&Message[oldSize], MessageSize);
			if (HasMask) for (auto i = 0; i < MessageSize; i++) Message[i] ^= Mask[i & 3];
		}
		return sz + Message.size();
	}
	WebSocket::WebSocket() {
		assert(0 && "This sould not happen");
	}

	bool WebSocket::CanRead() {
		return Socket.CanRead();
	}

	int WebSocket::Receive(std::vector<unsigned char>& Data) {
		auto sz = Socket.ReceiveAsync(Data);
		return sz;
	}

	WebSocket::WebSocket(const TcpSocket & sock) :Socket{ sock } {}
	int WebSocket::operator()(std::function<void(WebSocketPacket&)> Event) {
		WebSocketPacket Content;
		Event(Content);
		size_t sz = Content.Content.size();
		unsigned char Header[10];

		Header[0] = Content.Opcode | 0x80;
		int HeaderSize;
		if (sz < 126) {
			HeaderSize = 2;
			Header[1] = sz;
		} else if (sz <= 0xffff) {
			HeaderSize = 4;
			Header[1] = 126;
			Header[2] = sz >> 8;
			Header[3] = sz & 0xff;
		} else {
			HeaderSize = 10;
			Header[1] = 127;

			unsigned char * pSz = (unsigned char*)&sz;
			Header[2] = pSz[7];
			Header[3] = pSz[6];
			Header[4] = pSz[5];
			Header[5] = pSz[4];
			Header[6] = pSz[3];
			Header[7] = pSz[2];
			Header[8] = pSz[1];
			Header[9] = pSz[0];
		}
		return Socket.Send(Header, HeaderSize) < 0 || Socket.Send(Content.Content) < 0;
	}
	void WebSocket::Disconnect() {
		Socket.Disconnect();
	}
	void WebSocketPacket::Text(const std::string & Text) {
		Opcode = 1;
		Content.resize(Text.size());
		memcpy(&Content[0], &Text[0], Text.size());
	}
	void WebSocketPacket::Text(const std::wstring & Text) {
		Opcode = 1;
		Content = PVX::Encode::UTF(Text);
	}
	void WebSocketPacket::Json(const PVX::JSON::Item & Json) {
		Text(PVX::JSON::stringify(Json));
	}
	void WebSocketPacket::Run(const std::wstring & Function, const PVX::JSON::Item & Params) {
		Json({ { Function, Params } });
	}
	void WebSocketPacket::Data(const void * data, size_t Size) {
		Opcode = 2;
		Content.resize(Size);
		memcpy(&Content[0], data, Size);
	}
	void WebSocketPacket::Data(const std::vector<unsigned char> & data) {
		Opcode = 2;
		Content = data;
	}

	struct PacketHeader{
		unsigned char Header[10];
		int HeaderSize;
	};

	PacketHeader MakePacketHeader(size_t ContentSize, int Opcode) {
		PacketHeader ret;

		ret.Header[0] = Opcode | 0x80;
		if (ContentSize < 126) {
			ret.HeaderSize = 2;
			ret.Header[1] = ContentSize;
		} else if (ContentSize <= 0xffff) {
			ret.HeaderSize = 4;
			ret.Header[1] = 126;
			ret.Header[2] = ContentSize >> 8;
			ret.Header[3] = ContentSize & 0xff;
		} else {
			ret.HeaderSize = 10;
			ret.Header[1] = 127;

			unsigned char* pSz = (unsigned char*)&ContentSize;
			ret.Header[2] = pSz[7];
			ret.Header[3] = pSz[6];
			ret.Header[4] = pSz[5];
			ret.Header[5] = pSz[4];
			ret.Header[6] = pSz[3];
			ret.Header[7] = pSz[2];
			ret.Header[8] = pSz[1];
			ret.Header[9] = pSz[0];
		}
		return ret;
	}

	void WebSocketServer::Send(const std::string & ConnectionId, std::function<void(WebSocketPacket&)> Event) {
		std::vector<std::string> ToDelete;
		{
			std::shared_lock<std::shared_mutex> lock{ ConnectionMutex };
			if (!Connections.count(ConnectionId))return;
			WebSocketPacket Content;
			Event(Content);

			auto [Header, HeaderSize] = MakePacketHeader(Content.Content.size(), Content.Opcode);

			auto& Socket = Connections.at(ConnectionId);
			if (Socket.Socket.Send(Header, HeaderSize) < 0 || Socket.Socket.Send(Content.Content) < 0)
				ToDelete.push_back(ConnectionId);
		}
		if(ToDelete.size()) {
			std::unique_lock<std::shared_mutex> lock{ ConnectionMutex };
			for (auto& id : ToDelete)
				CloseConnection(id);
		}
	}
	void WebSocketServer::SendGroup(const std::string & GroupName, std::function<void(WebSocketPacket&)> Event) {
		std::vector<std::string> ToDelete;
		{
			std::shared_lock<std::shared_mutex> lock{ ConnectionMutex };
			WebSocketPacket Content;
			Event(Content);

			auto [Header, HeaderSize] = MakePacketHeader(Content.Content.size(), Content.Opcode);

			auto& Group = ConnectionGroups[GroupName];
			for (auto& ConnectionId : Group) {
				auto& Socket = Connections.at(ConnectionId);
				if (Socket.Socket.Send(Header, HeaderSize) < 0 || Socket.Socket.Send(Content.Content) < 0)
					ToDelete.push_back(ConnectionId);
			}
		}
		if (ToDelete.size()) {
			for (auto& id : ToDelete)
				CloseConnection(id);
		}
	}
	void WebSocketServer::SendAll(std::function<void(WebSocketPacket&)> Event) {
		std::vector<std::string> ToDelete;
		{
			std::shared_lock<std::shared_mutex> lock{ ConnectionMutex };
			WebSocketPacket Content;
			Event(Content);

			auto [Header, HeaderSize] = MakePacketHeader(Content.Content.size(), Content.Opcode);

			for (auto& con : Connections) {
				auto& Socket = con.second;
				if (Socket.Socket.Send(Header, HeaderSize) < 0 || Socket.Socket.Send(Content.Content) < 0)
					ToDelete.push_back(con.first);
			}
		}
		if (ToDelete.size()) {
			for (auto& id : ToDelete)
				CloseConnection(id);
		}
	}

	void WebSocketServer::SendAllExceptOne(const std::string & Id, std::function<void(WebSocketPacket&)> Event) {
		std::vector<std::string> ToDelete;
		{
			std::shared_lock<std::shared_mutex> lock{ ConnectionMutex };
			WebSocketPacket Content;
			Event(Content);

			auto [Header, HeaderSize] = MakePacketHeader(Content.Content.size(), Content.Opcode);

			for (auto& con : Connections) {
				if (con.first == Id)continue;
				auto& Socket = con.second;
				if (Socket.Socket.Send(Header, HeaderSize) < 0 || Socket.Socket.Send(Content.Content) < 0)
					ToDelete.push_back(con.first);
			}
		}
		if (ToDelete.size()) {
			for (auto& id : ToDelete)
				CloseConnection(id);
		}
	}
	void WebSocketServer::SendGroupExceptOne(const std::string & Id, const std::string & GroupName, std::function<void(WebSocketPacket&)> Event) {
		std::vector<std::string> ToDelete;
		{
			std::shared_lock<std::shared_mutex> lock{ ConnectionMutex };
			WebSocketPacket Content;
			Event(Content);

			auto [Header, HeaderSize] = MakePacketHeader(Content.Content.size(), Content.Opcode);

			std::vector<std::string> ToDelete;
			auto& Group = ConnectionGroups[GroupName];
			for (auto& ConnectionId : Group) {
				if (ConnectionId == Id)continue;
				auto& Socket = Connections.at(ConnectionId);
				if (Socket.Socket.Send(Header, HeaderSize) < 0 || Socket.Socket.Send(Content.Content) < 0)
					ToDelete.push_back(ConnectionId);
			}
		}
		if (ToDelete.size()) {
			for (auto& id : ToDelete)
				CloseConnection(id);
		}
	}

	void WebSocketServer::Run(const std::string& ConnectionId, const std::wstring& Function, const PVX::JSON::Item& Params) {
		Send(ConnectionId, [&](WebSocketPacket& pk) {
			pk.Run(Function, Params);
		});
	}
	void WebSocketServer::RunGroup(const std::string& GroupName, const std::wstring& Function, const PVX::JSON::Item& Params) {
		SendGroup(GroupName, [&](WebSocketPacket& pk) {
			pk.Run(Function, Params);
		});
	}
	void WebSocketServer::RunAll(const std::wstring& Function, const PVX::JSON::Item& Params) {
		SendAll([&](WebSocketPacket& pk) {
			pk.Run(Function, Params);
		});
	}
	void WebSocketServer::RunAllExceptOne(const std::string& Id, const std::wstring& Function, const PVX::JSON::Item& Params) {
		SendAllExceptOne(Id, [&](WebSocketPacket& pk) {
			pk.Run(Function, Params);
		});
	}
	void WebSocketServer::RunGroupExceptOne(const std::string& Id, const std::string& GroupName, const std::wstring& Function, const PVX::JSON::Item& Params) {
		SendGroupExceptOne(Id, GroupName, [&](WebSocketPacket& pk) {
			pk.Run(Function, Params);
		});
	}

	static std::string MakeKey(std::wstring txt) {
		std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

		std::vector<char> msg;
		msg.reserve(GUID.size() + txt.size());

		for (auto g : txt) msg.push_back(g);
		for (auto g : GUID) msg.push_back(g);

		auto Out = PVX::Encrypt::SHA1_Algorithm().Update(msg)();

		return PVX::Encode::Base64(Out);
	}
	std::function<void(HttpRequest&, HttpResponse&)> WebSocketServer::GetHandler() {
		return [this](HttpRequest& req, HttpResponse& resp) {
			using namespace PVX;
			using namespace PVX::String;
			auto key = MakeKey(req.Headers["sec-websocket-key"]());
			std::wstring Key = PVX::Encode::ToString(key);

			resp.StatusCode = 101;
			resp[L"upgrade"] = L"websocket";
			resp[L"connection"] = L"upgrade";
			resp[L"sec-websocket-accept"] = Key;			
			resp[L"set-cookie"] = L"pvxWSId=" + Key;
			auto s = req.GetWebSocket();
			{
				std::unique_lock<std::mutex> lock{ ThreadCleanerMutex };
				ServingThreads[key] = std::thread([s, key, this] {
					WebSocket Socket = s;
					for (;;) {
						if (auto res = Socket.Receive(); res < 0 || Socket.Opcode == WebSocket::Opcode_Close) {
							std::unique_lock<std::shared_mutex> lock{ ConnectionMutex };
							CloseConnection(key);
							break;
						} else if (res > 0) {
							int type = Socket.Message[0];
							std::string Name;
							size_t sz;
							for (sz = 1; sz < Socket.Message.size() && Socket.Message[sz] != ':'; sz++) Name.push_back(Socket.Message[sz]);
							if (ClientActions.count(Name)) {
								if (type == 'j') {
									JSON::Item params = JSON::jsElementType::Null;
									if (Socket.Message.size() - sz - 1)
										params = PVX::JSON::parse(&Socket.Message[sz + 1], Socket.Message.size() - sz - 1);

									ClientActions[Name](params, key);
								} else if (type == 'b') {
									std::vector<unsigned char> data(Socket.Message.size() - sz - 1);
									memcpy(&data[0], &Socket.Message[sz + 1], Socket.Message.size() - sz - 1);
									ClientActionsRaw[Name](data, key);
								}
							}
						}
					}
				});
			}
			{
				std::unique_lock<std::shared_mutex> lock{ ConnectionMutex };
				Connections.insert({ key, s });
			}


			if (onConnect != nullptr)onConnect(key, Connections.at(key));
		};
	}

	std::function<void(HttpRequest&, HttpResponse&)> WebSocketServer::GetScriptHandler(const std::wstring & Url) {
		return [this, Url](HttpRequest& req, HttpResponse&resp) {
			std::wstringstream ret;
			ret << LR"raw((function (pvx, server, fncs) {
	function MakeSendFunction(name){
		let args = "";
		let idx = name.indexOf(':');
		if(idx!=-1){
			args = name.substring(idx+1).replace(/\s/g, "");
			name = name.substring(0,idx).trim();
		}
		let params = null;
		if(args.length)params =`let p={};[${args.split(',').map(c=> '"' + c.trim() + '"')}].forEach((c, i) => p[c] = arguments.length>i?arguments[i]:null);`;
		let f=`(function ${name}(${args}){if(!this.default)return;${params||""}this.ws.send("j${name}:"${params?"+JSON.stringify(p)":""});})`;
		return f;
	}
	pvx.pvxWebSockets = new function () {
		this.status = "initial";
		let TO=1000,t = this;
		t.Server = {default:true};
		t.Client = {};
		function SetState(s){
			t.status = s;
			t.$scope&&
				t.$scope.$applyAsync();
		}
		this.SetAngularScope = function(element){t.$scope = angular.element(document.querySelector(element)).scope().$root;};
		this.connect = function connect() {
			if(angular)this.SetAngularScope("[ng-controller]");
			return new Promise(function(resolve, reject){
				if (t.ws) {
					SetState("reconnecting");
					t.ws.close();
				}else{
					SetState("connecting");
				}
				let p = window.location.port,pc = window.location.protocol=="https:"?"wss://":"ws://";
				t.Server.ws = t.ws = new WebSocket(pc + window.location.hostname + (p==80?"":(":"+p)) + server);
				t.ws.onopen = function(e){
					t.Id = document.cookie.replace(/\s/g,"").split(";").filter(c => c.indexOf("pvxWSId")==0)[0].substring(8);
					TO = 1000;
					t.onopen&&t.onopen(e);
					SetState("connected");
					resolve(t.Id);
				}
				t.ws.onmessage = function (e) {
					let action = JSON.parse(e.data);
					Object.keys(action).forEach(c => t.Client[c]&&t.Client[c](action[c]));
					if(t.$scope)t.$scope.$applyAsync();
				}
				t.ws.onerror = function (e) {
					t.onerror&&t.onerror(e);
				}
				t.ws.onclose = function (e) {
					if(TO<10000)TO+=500;
					if(!t.onclose||!t.onclose(e)) setTimeout(function () { 
						t.connect(); 
					}, TO);
					SetState("disconnected");
				}
			});
		}
		if(fncs&&fncs.length) for(let f of fncs) t.Server[f.split(":")[0].trim()] = eval(MakeSendFunction(f));
	}
})(window, ")raw" << Url << L"\",[";

			ret << PVX::Encode::ToString(functions);

			ret << L"]);";
			resp.UtfData(ret.str(), L"script/javascript");
		};
	}

	void WebSocketServer::AddClientAction(const std::string & Name, std::function<void(PVX::JSON::Item&, const std::string&)> Action) {
		functions += (functions.size() ? "," : "") + ("\"" + Name + "\"");
		std::string name;
		for (auto i = 0; i < Name.size() && Name[i] != ':'; i++)
			name.push_back(Name[i]);
		ClientActions[name] = Action;
	}
	void WebSocketServer::AddClientActionRaw(const std::string Name, std::function<void(const std::vector<unsigned char>&, const std::string&)> Action) {
		ClientActionsRaw[Name] = Action;
	}
	void WebSocketServer::AddServerAction(const std::string Name, std::function<void(WebSocketPacket&)> Action) {
		ServerActions[Name] = Action;
	}
	void WebSocketServer::AddToGroup(const std::string & GroupName, const std::string & ConnectionId) {
		std::unique_lock<std::shared_mutex> lock{ ConnectionMutex };
		GroupConnections[GroupName].insert(ConnectionId);
		ConnectionGroups[ConnectionId].insert(GroupName);
	}

	void WebSocketServer::RemoveFromGroup(const std::string & GroupName, const std::string & ConnectionId) {
		std::unique_lock<std::shared_mutex> lock{ ConnectionMutex };
		GroupConnections[GroupName].erase(ConnectionId);
		ConnectionGroups[ConnectionId].erase(GroupName);
	}

	void WebSocketServer::OnConnect(std::function<void(const std::string&, WebSocket&)> fnc) {
		onConnect = fnc;
	}

	void WebSocketServer::OnDisconnect(std::function<void(const std::string&)> fnc) {
		onDisconnect = fnc;
	}

	void WebSocketServer::CloseConnection(const std::string & ConnectionId) {
		auto str = ConnectionId;

		if (ConnectionGroups.count(str)) {
			for (auto& c : ConnectionGroups)
				for (auto& g : c.second)
					GroupConnections[g].erase(str);
			ConnectionGroups.erase(str);
		}
		Connections.erase(str);

		if (onDisconnect != nullptr)onDisconnect(str);
		
		{
			std::lock_guard<std::mutex> lock{ ThreadCleanerMutex };
			CleanUpKeys.push_back(str);
		}
		ThreadCleaner_cv.notify_one();
	}
	void WebSocketServer::ThreadCleanerClb() {
		while (running || ServingThreads.size()) {
			std::unique_lock<std::mutex> lock{ ThreadCleanerMutex };
			ThreadCleaner_cv.wait(lock, [this] { return CleanUpKeys.size()||!running; });
			for (auto th : CleanUpKeys) {
				ServingThreads[th].join();
				ServingThreads.erase(th);
			}
			CleanUpKeys.clear();
		}
	}
	WebSocketServer::~WebSocketServer() {
		{
			std::unique_lock<std::shared_mutex> lock{ ConnectionMutex };
			for (auto& [c, s] : Connections) {
				s.Socket.Disconnect();
			}
		}
		running = false;
		ThreadCleaner_cv.notify_one();
		ThreadCleanerThread.join();
	}
	WebSocketServer::WebSocketServer() : running{ 1 }, ThreadCleanerThread(&WebSocketServer::ThreadCleanerClb, this) {}
}