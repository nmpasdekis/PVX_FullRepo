#pragma once
#include <PVX_Network.h>
#include <PVX_json.h>
#include <atomic>

namespace PVX::MongoDB {
	class Collection_t {
	public:
		PVX::JSON::Item Find(const JSON::Item& Query = nullptr, const JSON::Item& Selector = nullptr, int32_t skip = 0, int32_t take = 0);
		PVX::JSON::Item FindOne(const JSON::Item& Query = nullptr, const JSON::Item& Selector = nullptr, int32_t skip = 0);
		int Update(const JSON::Item& update, const JSON::Item& selector = nullptr);
		int UpdateMany(const JSON::Item& update, const JSON::Item& selector = nullptr);
		int Upsert(const JSON::Item& update, const JSON::Item& selector = nullptr);
		int UpsertMany(const JSON::Item& update, const JSON::Item& selector = nullptr);
		int Insert(const JSON::Item& document);
		int Insert(const std::initializer_list<JSON::Item>& document);
		int Delete(const JSON::Item& selector);
		int DeleteMany(const JSON::Item& selector);

		int32_t QueryFlags = 0;
		int32_t InsertFlags = 1;
	protected:
		Collection_t(const PVX::Network::TcpSocket& Socket, const std::wstring& Collection, std::atomic_int32_t& ReqId);
		std::atomic_int32_t& RequestId;
		int32_t AfterCollection;
		PVX::Network::TcpSocket Socket;
		std::vector<unsigned char> SendBuffer;
		std::vector<unsigned char> ReceiveBuffer;


		void Make_OP_QUERY(const JSON::Item& Query, const JSON::Item& Selector, int32_t skip, int32_t take);
		void Make_OP_GET_MORE();
		void Make_OP_INSERT(const std::initializer_list<JSON::Item>& documents);
		void Make_OP_UPDATE(const JSON::Item& selector, const JSON::Item& update, int32_t flags);
		void Make_OP_DELETE(const JSON::Item& selector, int32_t flags);

		void AppendSendDoc(const JSON::Item& doc);

		template<typename T>
		T& GetMoreSendHeader() { return *(T*)&SendBuffer[AfterCollection]; }

		friend class Database;
	};

	class Database {
	public:
		Database(const std::wstring& Database, const char* Url = "localhost", unsigned short Port = 27017);
		Collection_t Collection(const std::wstring& Collection);
		PVX::JSON::Item RunCommand(const JSON::Item& Command, const std::string& DocumentIdentifier = "documents", const std::initializer_list<JSON::Item>& Documents = {}, uint32_t Flags = 0);
		PVX::JSON::Item RunCommand_Full(const JSON::Item& Command, const std::string& DocumentIdentifier = "documents", const std::initializer_list<JSON::Item>& Documents = {}, uint32_t Flags = 0);
	private:
		std::atomic_int32_t RequestId = 1;
		PVX::Network::TcpSocket Socket;
		std::wstring DatabaseName;
		std::vector<unsigned char> SendMsgBuffer;
		std::vector<unsigned char> ReceiveBuffer;

		void Make_OP_MSG(const JSON::Item& Command, const std::string& DocumentIdentifier, const std::initializer_list<JSON::Item>& Documents, uint32_t Flags);
		void AppendSendMsgDoc(const JSON::Item& doc);
	};
}