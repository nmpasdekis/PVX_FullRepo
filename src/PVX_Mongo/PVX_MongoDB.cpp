#include <PVX_MongoDb.h>

#pragma comment (lib, "PVX_General.lib")
#pragma comment (lib, "PVX_Network.lib")

namespace PVX::MongoDB {
	enum class Opcode : int32_t {
		OP_REPLY = 1,
		OP_UPDATE = 2001,
		OP_INSERT = 2002,
		RESERVED = 2003,
		OP_QUERY = 2004,
		OP_GET_MORE = 2005,
		OP_DELETE = 2006,
		OP_KILL_CURSORS = 2007,
		OP_MSG = 2013
	};

#pragma pack(push, 1)
	struct MsgHeader {
		int32_t   messageLength;
		int32_t   requestID; 
		int32_t   responseTo;
		Opcode   opCode;        
	};

	struct OP_REPLY {
		MsgHeader header;
		int32_t     responseFlags;
		int64_t     cursorID;
		int32_t     startingFrom;
		int32_t     numberReturned;
	};

	struct OP_Header {
		MsgHeader header;
		int32_t   flags;
	};

	struct OP_QUERY_more {
		int32_t     numberToSkip;
		int32_t     numberToReturn;
	};

	struct OP_GET_MORE_more {
		int32_t     numberToReturn;
		int64_t     cursorID;
	};

	struct OP_UPDATE_more {
		int32_t     flags;
	};

	struct OP_DELETE_more {
		int32_t     flags;
	};

#pragma pack(pop)
	Database::Database(const std::wstring& Database, const char* Url, unsigned short Port) : DatabaseName{ Database } {
		Socket.Connect(Url, std::to_string(Port).c_str());
	}
	Collection_t Database::Collection(const std::wstring& CollectionName) {
		return Collection_t(Socket, DatabaseName + L"." + CollectionName, RequestId);
	}
	Collection_t::Collection_t(const PVX::Network::TcpSocket& Socket, const std::wstring& Collection, std::atomic_int32_t& ReqId) :Socket{ Socket }, RequestId{ ReqId } {
		SendBuffer.resize(sizeof(OP_Header) + PVX::Encode::UTF_Length(Collection) + 1);
		PVX::Encode::UTF(&SendBuffer[sizeof(OP_Header)], Collection.c_str());
		AfterCollection = SendBuffer.size();
	}

	void Database::AppendSendMsgDoc(const JSON::Item& doc) {
		PVX::JSON::ToBSON(doc, SendMsgBuffer);
	}
	void Collection_t::AppendSendDoc(const JSON::Item& doc) {
		PVX::JSON::ToBSON(doc, SendBuffer);
	}

	void Collection_t::Make_OP_GET_MORE() {
		SendBuffer.resize(AfterCollection + sizeof(OP_GET_MORE_more));
		OP_Header& qq = *(OP_Header*)SendBuffer.data();
		qq.header.messageLength = SendBuffer.size();
		qq.header.requestID = ++RequestId;
		qq.header.responseTo = 0;
		qq.header.opCode = Opcode::OP_GET_MORE;
		qq.flags = 0;
	}

	void Collection_t::Make_OP_INSERT(const std::initializer_list<JSON::Item>& documents) {
		SendBuffer.resize(AfterCollection);
		for(auto& document: documents)
			AppendSendDoc(document);
		OP_Header& qq = *(OP_Header*)SendBuffer.data();
		qq.header.messageLength = SendBuffer.size();
		qq.header.requestID = ++RequestId;
		qq.header.responseTo = 0;
		qq.header.opCode = Opcode::OP_INSERT;
		qq.flags = InsertFlags;
	}

	void Collection_t::Make_OP_UPDATE(const JSON::Item& selector, const JSON::Item& update, int32_t flags) {
		SendBuffer.resize(AfterCollection + sizeof(OP_UPDATE_more));
		auto& More = GetMoreSendHeader<OP_UPDATE_more>();
		More.flags = flags;
		AppendSendDoc(selector||JSON::jsElementType::Object);
		AppendSendDoc(update);

		OP_Header& qq = *(OP_Header*)SendBuffer.data();
		qq.header.messageLength = SendBuffer.size();
		qq.header.requestID = ++RequestId;
		qq.header.responseTo = 0;
		qq.header.opCode = Opcode::OP_UPDATE;
		qq.flags = 0;
	}

	void Collection_t::Make_OP_DELETE(const JSON::Item& selector, int32_t flags) {
		SendBuffer.resize(AfterCollection + sizeof(OP_UPDATE_more));
		auto& More = GetMoreSendHeader<OP_UPDATE_more>();
		More.flags = 0;
		AppendSendDoc(selector||JSON::jsElementType::Object);

		OP_Header& qq = *(OP_Header*)SendBuffer.data();
		qq.header.messageLength = SendBuffer.size();
		qq.header.requestID = ++RequestId;
		qq.header.responseTo = 0;
		qq.header.opCode = Opcode::OP_DELETE;
		qq.flags = flags;
	}

	void Collection_t::Make_OP_QUERY(const JSON::Item& Query, const JSON::Item& Selector, int32_t skip, int32_t take) {
		SendBuffer.resize(AfterCollection + sizeof(OP_QUERY_more));

		auto& More = GetMoreSendHeader<OP_QUERY_more>();
		More.numberToReturn = take;
		More.numberToSkip = skip;

		AppendSendDoc(Query || JSON::jsElementType::Object);
		AppendSendDoc(Selector);
		OP_Header& qq = *(OP_Header*)SendBuffer.data();
		qq.header.messageLength = SendBuffer.size();
		qq.header.requestID = ++RequestId;
		qq.header.responseTo = 0;
		qq.header.opCode = Opcode::OP_QUERY;
		qq.flags = QueryFlags;
	}

	PVX::JSON::Item Collection_t::Find(const JSON::Item& Query, const JSON::Item& Selector, int32_t skip, int32_t take) {
		Make_OP_QUERY(Query, Selector, skip, take);

		if (!Socket.Send(SendBuffer)) return JSON::jsElementType::Undefined;
		ReceiveBuffer.clear();
		if (Socket.Receive(ReceiveBuffer)) {
			OP_REPLY* Header = (OP_REPLY*)ReceiveBuffer.data();

			JSON::Item ret = JSON::jsElementType::Array;
			auto& Array = ret.getArray();
			if (take) Array.reserve(take);
			else Array.reserve(Header->numberReturned);


			size_t cur = sizeof(OP_REPLY);
			while (cur<Header->header.messageLength) {
				Array.emplace_back(std::move(JSON::fromBSON(ReceiveBuffer, cur)));
			}
			if (Header->cursorID && (take == 0 || Array.size() < take)) {
				Make_OP_GET_MORE();
				auto& More2 = GetMoreSendHeader<OP_GET_MORE_more>();
				More2.numberToReturn = take ? (take - Array.size()) : 0;
				while (Header->cursorID) {
					More2.cursorID = Header->cursorID;
					ReceiveBuffer.clear();
					if (!Socket.Send(SendBuffer) || !Socket.Receive(ReceiveBuffer)) break;

					Header = (OP_REPLY*)ReceiveBuffer.data();
					if(!take)Array.reserve(Header->numberReturned + Array.size());
					cur = sizeof(OP_REPLY);
					while (cur<Header->header.messageLength) {
						Array.emplace_back(std::move(JSON::fromBSON(ReceiveBuffer, cur)));
					}
				}
			}
			return ret;
		}
		return JSON::jsElementType::Undefined;
	}
	PVX::JSON::Item Collection_t::FindOne(const JSON::Item& Query, const JSON::Item& Selector, int32_t skip) {
		Make_OP_QUERY(Query, Selector, skip, 1);

		if (!Socket.Send(SendBuffer)) return JSON::jsElementType::Undefined;
		ReceiveBuffer.clear();
		if (Socket.Receive(ReceiveBuffer)) {
			OP_REPLY* Header = (OP_REPLY*)ReceiveBuffer.data();
			size_t cur = sizeof(OP_REPLY);
			if(Header->numberReturned == 1) 
				return JSON::fromBSON(ReceiveBuffer, cur);
		}
		return JSON::jsElementType::Undefined;
	}
	int Collection_t::Insert(const JSON::Item& document) {
		Make_OP_INSERT({ document });
		return Socket.Send(SendBuffer);
	}
	int Collection_t::Insert(const std::initializer_list<JSON::Item>& document) {
		Make_OP_INSERT(document);
		return Socket.Send(SendBuffer);
	}
	int Collection_t::Update(const JSON::Item& update, const JSON::Item& selector) {
		Make_OP_UPDATE(selector, update, 0);
		return Socket.Send(SendBuffer);
	}
	int Collection_t::Upsert(const JSON::Item& update, const JSON::Item& selector) {
		Make_OP_UPDATE(selector, update, 1);
		return Socket.Send(SendBuffer);
	}
	int Collection_t::UpdateMany(const JSON::Item& update, const JSON::Item& selector) {
		Make_OP_UPDATE(selector, update, 2);
		return Socket.Send(SendBuffer);
	}
	int Collection_t::UpsertMany(const JSON::Item& update, const JSON::Item& selector) {
		Make_OP_UPDATE(selector, update, 3);
		return Socket.Send(SendBuffer);
	}
	int Collection_t::Delete(const JSON::Item& selector) {
		Make_OP_DELETE(selector, 0);
		return Socket.Send(SendBuffer);
	}
	int Collection_t::DeleteMany(const JSON::Item& selector) {
		Make_OP_DELETE(selector, 1);
		return Socket.Send(SendBuffer);
	}

	void Database::Make_OP_MSG(const JSON::Item& Command, const std::string& DocId, const std::initializer_list<JSON::Item>& Documents, uint32_t Flags) {
		SendMsgBuffer.resize(sizeof(OP_Header));
		SendMsgBuffer.push_back(0);
		AppendSendMsgDoc(Command);
		if (Documents.size()) {
			SendMsgBuffer.push_back(1);
			auto SizeIndex = SendMsgBuffer.size();
			SendMsgBuffer.resize(SizeIndex + sizeof(int32_t) + DocId.size() + 1);
			memcpy(&SendMsgBuffer[SizeIndex + sizeof(int32_t)], DocId.data(), DocId.size() + 1);
			for (auto& doc: Documents) AppendSendMsgDoc(doc);
			*(int32_t*)&SendMsgBuffer[SizeIndex] = SendMsgBuffer.size() - SizeIndex;
		}
		OP_Header& qq = *(OP_Header*)SendMsgBuffer.data();
		qq.header.messageLength = SendMsgBuffer.size();
		qq.header.requestID = ++RequestId;
		qq.header.responseTo = 0;
		qq.header.opCode = Opcode::OP_MSG;
		qq.flags = Flags;
	}

	PVX::JSON::Item Database::RunCommand_Full(const JSON::Item& cmd, const std::string& DocumentIdentifier, const std::initializer_list<JSON::Item>& Documents, uint32_t Flags) {
		JSON::Item Command = cmd;
		Command[L"$db"] = DatabaseName;
		Make_OP_MSG(Command, DocumentIdentifier, Documents, Flags);
		ReceiveBuffer.clear();
		if(!Socket.Send(SendMsgBuffer) || !Socket.Receive(ReceiveBuffer)) 
			return 0;

		OP_Header& Header1 = *(OP_Header*)ReceiveBuffer.data();
		size_t cur = sizeof(OP_Header) + 1;
		auto Cmd = JSON::fromBSON(ReceiveBuffer, cur);

		while (cur < ReceiveBuffer.size() && ReceiveBuffer[cur] == 1) {
			auto Start = ++cur;
			int32_t Size = *(int32_t*)&ReceiveBuffer[cur];
			cur += sizeof(int32_t);
			std::string DocId = (char*)&ReceiveBuffer[cur];
			cur += DocId.size() + 1;
			auto docs = (Cmd[DocId] = JSON::jsElementType::Array).getArray();
			while (cur - Start < Size) {
				docs.push_back(JSON::fromBSON(ReceiveBuffer, cur));
			}
		}
		return std::move(Cmd);
	}
	PVX::JSON::Item Database::RunCommand(const JSON::Item& Command, const std::string& DocumentIdentifier, const std::initializer_list<JSON::Item>& Documents, uint32_t Flags) {
		auto res = RunCommand_Full(Command, DocumentIdentifier, Documents, Flags);
		if (res[L"ok"].Double()) return res[L"cursor"]["firstBatch"];
		return res;
	}
}