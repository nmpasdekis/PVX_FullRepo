#include <PVX_MSSQL.h>
#include <PVX_Encode.h>
#include <PVX_Regex.h>
#include <PVX_String.h>
#include <PVX.inl>

namespace PVX {
	namespace DB {
		static std::map<SQLSMALLINT, const std::string> VarTypeMap{
			{ SQL_UNKNOWN_TYPE, "SQL_UNKNOWN_TYPE" },
			{ SQL_CHAR, "SQL_CHAR" },
			{ SQL_NUMERIC, "SQL_NUMERIC" },
			{ SQL_DECIMAL, "SQL_DECIMAL" },
			{ SQL_INTEGER, "SQL_INTEGER" },
			{ SQL_SMALLINT, "SQL_SMALLINT" },
			{ SQL_FLOAT, "SQL_FLOAT" },
			{ SQL_REAL, "SQL_REAL" },
			{ SQL_DOUBLE, "SQL_DOUBLE" },
			{ SQL_DATETIME, "SQL_DATETIME" },
			{ SQL_VARCHAR, "SQL_VARCHAR" },
			{ SQL_TYPE_DATE, "SQL_TYPE_DATE" },
			{ SQL_TYPE_TIME, "SQL_TYPE_TIME" },
			{ SQL_TYPE_TIMESTAMP, "SQL_TYPE_TIMESTAMP" },
			{ SQL_DATE, "SQL_DATE" },
			{ SQL_INTERVAL, "SQL_INTERVAL" },
			{ SQL_TIME, "SQL_TIME" },
			{ SQL_TIMESTAMP, "SQL_TIMESTAMP" },
			{ SQL_LONGVARCHAR, "SQL_LONGVARCHAR" },
			{ SQL_BINARY, "SQL_BINARY" },
			{ SQL_VARBINARY, "SQL_VARBINARY" },
			{ SQL_LONGVARBINARY, "SQL_LONGVARBINARY" },
			{ SQL_BIGINT, "SQL_BIGINT" },
			{ SQL_TINYINT, "SQL_TINYINT" },
			{ SQL_BIT, "SQL_BIT" },
			{ SQL_GUID, "SQL_GUID" },
			{ SQL_UNICODE, "SQL_UNICODE" },
			{ SQL_UNICODE_VARCHAR, "SQL_UNICODE_VARCHAR" },
			{ SQL_UNICODE_LONGVARCHAR, "SQL_UNICODE_LONGVARCHAR" },
			{ SQL_UNICODE_CHAR, "SQL_UNICODE_CHAR" },
		};

		static std::map<SQLSMALLINT, SQLSMALLINT> VarTypeMapToC{
			{ SQL_CHAR, SQL_C_CHAR },
			{ SQL_NUMERIC, SQL_C_BINARY },
			{ SQL_DECIMAL, SQL_C_DOUBLE },
			{ SQL_INTEGER, SQL_C_LONG },
			{ SQL_SMALLINT, SQL_C_SHORT },
			{ SQL_FLOAT, SQL_C_DOUBLE },
			{ SQL_REAL, SQL_C_DOUBLE },
			{ SQL_DOUBLE, SQL_C_DOUBLE },
			{ SQL_DATETIME, SQL_C_BINARY },				// 0
			{ SQL_VARCHAR, SQL_C_BINARY },				// 0
			{ SQL_TYPE_DATE, SQL_C_TYPE_DATE },
			{ SQL_TYPE_TIME, SQL_C_TYPE_TIME },
			{ SQL_TYPE_TIMESTAMP, SQL_C_TYPE_TIMESTAMP },
			{ SQL_DATE, SQL_C_DATE },
			{ SQL_INTERVAL, SQL_C_BINARY },				// 0
			{ SQL_TIME, SQL_C_TIME },
			{ SQL_TIMESTAMP, SQL_C_TIMESTAMP },
			{ SQL_LONGVARCHAR, SQL_C_BINARY },			// 0
			{ SQL_BINARY, SQL_C_BINARY },
			{ SQL_VARBINARY, SQL_C_BINARY },
			{ SQL_LONGVARBINARY, SQL_C_BINARY },
			{ SQL_BIGINT, SQL_C_SBIGINT },
			{ SQL_TINYINT, SQL_C_TINYINT },
			{ SQL_BIT, SQL_C_BIT },
			{ SQL_GUID, SQL_C_GUID },
			{ SQL_UNICODE, SQL_C_BINARY },				// 0
			{ SQL_UNICODE_VARCHAR, SQL_C_BINARY },		// 0
			{ SQL_UNICODE_LONGVARCHAR, SQL_C_BINARY },	// 0
			{ SQL_UNICODE_CHAR, SQL_C_BINARY },			// 0


			//{ SQL_CHAR, SQL_C_CHAR },
			//{ SQL_NUMERIC, SQL_C_DOUBLE },
			//{ SQL_DECIMAL, 0 },
			//{ SQL_INTEGER, SQL_C_LONG },
			//{ SQL_SMALLINT, SQL_C_SHORT },
			//{ SQL_FLOAT, SQL_C_DOUBLE },
			//{ SQL_REAL, SQL_C_DOUBLE },
			//{ SQL_DOUBLE, SQL_C_DOUBLE },
			//{ SQL_DATETIME, 0 },
			//{ SQL_VARCHAR, 0 },
			//{ SQL_TYPE_DATE, 0 },
			//{ SQL_TYPE_TIME, 0 },
			//{ SQL_TYPE_TIMESTAMP, 0 },
			//{ SQL_DATE, 0 },
			//{ SQL_INTERVAL, 0 },
			//{ SQL_TIME, 0 },
			//{ SQL_TIMESTAMP, 0 },
			//{ SQL_LONGVARCHAR, 0 },
			//{ SQL_BINARY, SQL_C_BINARY },
			//{ SQL_VARBINARY, SQL_C_BINARY },
			//{ SQL_LONGVARBINARY, SQL_C_BINARY },
			//{ SQL_BIGINT, SQL_C_SBIGINT },
			//{ SQL_TINYINT, SQL_C_TINYINT },
			//{ SQL_BIT, "SQL_BIT" },
			//{ SQL_GUID, "SQL_GUID" },
			//{ SQL_UNICODE, "SQL_UNICODE" },
			//{ SQL_UNICODE_VARCHAR, "SQL_UNICODE_VARCHAR" },
			//{ SQL_UNICODE_LONGVARCHAR, "SQL_UNICODE_LONGVARCHAR" },
			//{ SQL_UNICODE_CHAR, "SQL_UNICODE_CHAR" },
		};

		std::string GetError(SQLHSTMT hStmt) {
			SQLCHAR State[1024];
			SQLINTEGER    NativeErrorPtr;
			SQLCHAR       MessageText[1024];
			SQLSMALLINT   BufferLength;
			SQLSMALLINT   TextLengthPtr;
			SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, 1, State, &NativeErrorPtr, MessageText, 1023, &TextLengthPtr);
			return (char*)MessageText;
		}

		std::vector<ParamTuple> ToParams(const PVX::JSON::Item & v) {
			std::vector<ParamTuple> ret;
			for (auto &[Name, Value] : v.getObject())
				ret.push_back({ Name, Value });
			return ret;
		}

		void SQL::SetConnectionString(const std::string & Connection) {
			DefaultConnectionString = Connection;
		}
		void SQL::SetConnectionString(const std::string & Database, const std::string & Username, const std::string & Password, const std::string & Server) {
			DefaultConnectionString = "DRIVER={SQL Server};SERVER=" + Server + ";DATABASE=" + Database + ";UID=" + Username + ";PWD=" + Password + ";";
		}

		std::string SQL::DefaultConnectionString;

		SQL::SQL() :SQL(DefaultConnectionString) {}

		SQL::SQL(const std::string & connectionString) {
			auto ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
			ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
			ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
			ret = SQLDriverConnectA(hDbc, GetDesktopWindow(), (SQLCHAR*)&connectionString[0], SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
		}

		SQL::SQL(const std::string & Database, const std::string & Username, const std::string & Password, const std::string & Server) :
			SQL("DRIVER={SQL Server};SERVER=" + Server + ";DATABASE=" + Database + ";UID=" + Username + ";PWD=" + Password + ";") {}

		SQL::~SQL() {
			for (auto & h : Statements)
				SQLFreeHandle(SQL_HANDLE_STMT, h);

			if (hDbc) {
				SQLDisconnect(hDbc);
				SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
			}

			if (hEnv)
				SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
		}

		static std::wregex QueryParamRegex(L"@[_0-9a-zA-Z]+", std::regex_constants::optimize | std::regex_constants::ECMAScript);
		struct _BigParam {
			int cur;
			std::vector<unsigned char> Data;
		};
		SQLHSTMT SQL::Exec(const std::wstring & qUery, const std::vector<ParamTuple> & Params) {
			std::map<std::wstring, Parameter> params;
			for (auto & p : Params) {
				params[p.Name] = p.Param;
			}

			std::vector<std::wstring> paramSlots;
			PVX::onMatch(qUery, QueryParamRegex, [&paramSlots](const std::wsmatch & m) {
				paramSlots.push_back(m.str().substr(1));
			});
			std::wstring query = qUery;

			for (auto & p : paramSlots) {
				query = PVX::Replace(query, L"@" + p, L"?");
			}

			auto q = PVX::Encode::UTF(query);
			q.push_back(0);
			SQLHSTMT hStmt;
			SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
			SQLPrepareA(hStmt, &q[0], SQL_NTS);


			SQLLEN     LenOrInd;
			int Index = 1;
			SQLRETURN paramRet;
			std::vector<_BigParam> BigParams;
			char Null = 0;
			for (auto & p : paramSlots) {
				Parameter & pp = params[p];
				if (!pp.IsBig) {
					if(pp.Data.size())
						paramRet = SQLBindParameter(hStmt, Index++, SQL_PARAM_INPUT, pp.cType, pp.Type, pp.ColumnSize, 0, pp.Data.data(), pp.BufferSize, &pp.len);
					else
						paramRet = SQLBindParameter(hStmt, Index++, SQL_PARAM_INPUT, pp.cType, pp.Type, 1, 0, &Null, 1, &pp.len);
				} else {
					int id = BigParams.size() + 1;
					BigParams.push_back({ 0, pp.Data });
					//auto tp = SQLGetTypeInfo(hStmt, pp.Type);
					paramRet = SQLBindParameter(hStmt, Index++, SQL_PARAM_INPUT, pp.cType, pp.Type, pp.BufferSize, 0, (PTR)id, 0, &pp.len);
				}
			}
			Result = SQLExecute(hStmt);

			while(Result == SQL_NEED_DATA){
				SQLPOINTER pToken = NULL;
				Result = SQLParamData(hStmt, &pToken);
				if (Result == SQL_NEED_DATA) {
					auto & bdt = BigParams[((int)pToken) - 1];
					while (bdt.cur < bdt.Data.size()) {
						int sz = bdt.Data.size() - bdt.cur;
						if (sz > 1000) sz = 1000;

						auto dtRes = SQLPutData(hStmt, bdt.Data.data() + bdt.cur, sz);
						bdt.cur += sz;
					}
				}
			}

			if (Result != 0) {
				Error = GetError(hStmt);
			}
			Statements.push_back(hStmt);
			return hStmt;
		}

		SQL_Reader SQL::Query(const std::wstring & query, const std::vector<ParamTuple> & params) {
			return Exec(query, params);
		}
		SQLRETURN SQL::Execute(const std::wstring & query, const std::vector<ParamTuple> & params) {
			Exec(query, params);
			return Result;
		}

		std::wstring SQL::stringQuery(const std::wstring & query, const std::vector<ParamTuple> & params) {
			auto r = Query(L"SELECT (" + query + L"\nFOR JSON AUTO) json", params);
			if (r.Read() && !r.IsNullOrEmpty(0)) return r[0].WCharArray;
			return L"[]";
		}
		std::wstring SQL::stringQueryJson(const std::wstring & query, const PVX::JSON::Item & params) {
			auto r = Query(L"SELECT (" + query + L"\nFOR JSON AUTO) json", ToParams(params));
			if (r.Read() && !r.IsNullOrEmpty(0)) return r[0].WCharArray;
			return L"[]";
		}
		long long SQL::Exist(const std::wstring & Table, const std::vector<ParamTuple>& Values, const std::wstring& Where) {
			std::wstring q = L"SELECT count(*) 'Count'\nFROM " + Table;
			if (Where.size()) q += L"\nWHERE " + Where;
			else q += L"\nWHERE " + PVX::String::Join(Values, L" AND ", [](const ParamTuple& p) { return L"[" + p.Name + L"] = @" + p.Name; });
			auto r = Query(q, Values);
			if (r.Read())
				return r[0].Int;
			return 0;
		}

		SQL_Reader SQL::Update(const std::wstring & Table, const std::vector<ParamTuple>& Values, const std::set<std::wstring>& ConditionParams, const std::wstring & Where, const std::vector<std::wstring>& ReturnFields) {
			std::wstring q = L"UPDATE " + Table + L"\nSET " + PVX::String::Join(PVX::Filter(Values, [&ConditionParams](const auto & p) {
				return !ConditionParams.count(p.Name);
			}), L", ", [&ConditionParams](const ParamTuple & p) {
				return L"[" + p.Name + L"] = @" + p.Name;
			});
			if(ReturnFields.size())
				q += L"\nOUTPUT " + PVX::String::Join(ReturnFields, L",", [](auto p) { return L"inserted." + p; }) + L"\n";
			if (Where.size()) {
				q += L"\nWHERE " + Where;
			} else if (ConditionParams.size()) {
				q += L"\nWHERE " + PVX::String::Join(ConditionParams, L" AND ", [](const std::wstring& p) {
					return L"[" + p + L"] = @" + p;
				});
			}
			return Exec(q, Values);
		}

		SQL_Reader SQL::Delete(const std::wstring & Table, const std::vector<ParamTuple>& Params, const std::wstring & Where, const std::vector<std::wstring>& ReturnFields) {
			std::wstring q = L"DELETE FROM " + Table + L"\n";
			q += L"OUTPUT " + PVX::String::Join(ReturnFields, L",", [](auto p) { return L"inserted." + p; }) + L"\n"; 
			if (Where.size())
				q += Where;
			else
				q += L"WHERE " + PVX::String::Join(Params, L" AND ", [](const ParamTuple& p) { return L"[" + p.Name + L"] = @" + p.Name; });
			return Exec(q, Params);
		}
		PVX::JSON::Item SQL::JsonDelete(const std::wstring& Table, const std::vector<ParamTuple> & Params, const std::wstring& Where, const std::vector<std::wstring>& ReturnFields) {
			return ReaderToJson(Delete(Table, Params, Where, ReturnFields));
		}
		PVX::JSON::Item SQL::JsonDeleteJson(const std::wstring& Table, const PVX::JSON::Item& Params, const std::wstring& Where, const std::vector<std::wstring>& ReturnFields) {
			return ReaderToJson(Delete(Table, ToParams(Params), Where, ReturnFields));
		}
		SQL_Reader SQL::Insert(const std::wstring & Table, const std::vector<ParamTuple>& Params, const std::vector<std::wstring>& ReturnFields) {
			std::wstring q = L"INSERT INTO " + Table + L" (" + PVX::String::Join(Params, L",", [](auto p) { return L"[" + p.Name + L"]"; }) + L")\n";
				if (ReturnFields.size()) {
					q += L"OUTPUT " + PVX::String::Join(ReturnFields, L",", [](auto p) { return L"inserted." + p; }) + L"\n";
				}
				q += L"VALUES(" + PVX::String::Join(Params, L",", [](auto p) { return L"@" + p.Name; }) + L")";
			return Exec(q, Params);
		}

		PVX::JSON::Item SQL::JsonInsert(const std::wstring & Table, const std::vector<ParamTuple>& Params, const std::vector<std::wstring>& ReturnFields) {
			return ReaderToJson(Insert(Table, Params, ReturnFields));
		}
		PVX::JSON::Item SQL::JsonInsertJson(const std::wstring & Table, const PVX::JSON::Item & Params, const std::vector<std::wstring>& ReturnFields) {
			return JsonInsert(Table, ToParams(Params), ReturnFields);
		}

		long long SQL::ExistJson(const std::wstring & Table, const PVX::JSON::Item & Values, const std::wstring & Where) {
			return Exist(Table, ToParams(Values), Where);
		}

		SQL_Reader SQL::UpdateJson(const std::wstring & Table, const PVX::JSON::Item & Values, const std::set<std::wstring>& ConditionParams, const std::wstring & Where, const std::vector<std::wstring>& ReturnFields) {
			return Update(Table, ToParams(Values), ConditionParams, Where, ReturnFields);
		}

		PVX::JSON::Item SQL::JsonUpdateJson(const std::wstring & Table, const PVX::JSON::Item & Values, const std::set<std::wstring>& ConditionParams, const std::wstring & Where, const std::vector<std::wstring>& ReturnFields) {
			return ReaderToJson(Update(Table, ToParams(Values), ConditionParams, Where, ReturnFields));
		}
		PVX::JSON::Item SQL::JsonUpdate(const std::wstring & Table, const const std::vector<ParamTuple> & Values, const std::set<std::wstring>& ConditionParams, const std::wstring & Where, const std::vector<std::wstring>& ReturnFields) {
			return ReaderToJson(Update(Table, Values, ConditionParams, Where, ReturnFields));
		}
		SQL_Reader SQL::DeleteJson(const std::wstring & Table, const PVX::JSON::Item & Params, const std::wstring & Where, const std::vector<std::wstring>& ReturnFields) {
			return Delete(Table, ToParams(Params), Where, ReturnFields);
		}

		SQL_Reader SQL::InsertJson(const std::wstring & Table, const PVX::JSON::Item & Params, const std::vector<std::wstring>& ReturnFields) {
			return Insert(Table, ToParams(Params), ReturnFields);
		}

		//PVX::JSON::Item SQL::JsonUpsertJson(const std::wstring & Table, const PVX::JSON::Item & Params, const std::set<std::wstring>& ConditionParams, bool InsertKeys) {
		//	return JsonUpsert(Table, ToParams(Params), ConditionParams, InsertKeys);
		//}

		PVX::JSON::Item SQL::SelectJson(const std::wstring & Table, const std::vector<std::wstring>& Fields, const std::set<std::wstring>& JsonFields, const std::wstring & Where) {
			auto[primitive, json] = PVX::Separate(Fields, [JsonFields](const std::wstring & f) { return !JsonFields.count(f); });
			std::wstring q = L"SELECT " + PVX::String::Join(primitive, L",", [](const std::wstring & f) { return L"[" + f + L"]"; });
			if (json.size()) q += (primitive.size() ? L"," : L"") + PVX::String::Join(json, L",", [](const std::wstring & f) { return L"JSON_QUERY([" + f + L"], '$') [" + f + L"]"; });
			q += L"\nFROM " + Table;
			if (Where.size()) q += L"\nWHERE " + Where;
			return SelectJson(q);
		}

		PVX::JSON::Item SQL::SelectJson(const std::wstring & query, const std::vector<ParamTuple>& params) {
			auto r = Query(L"SELECT (" + query + L"\nFOR JSON AUTO) json", params);
			if (r.Read() && !r.IsNull(0)) return PVX::JSON::parse(r[0].WCharArray);
			return PVX::JSON::jsElementType::Array;
		}

		std::wstring SQL::StringSelectJson(const std::wstring & Table, const std::vector<std::wstring>& Fields, const std::set<std::wstring>& JsonFields, const std::wstring & Where) {
			auto[primitive, json] = PVX::Separate(Fields, [JsonFields](const std::wstring & f) { return !JsonFields.count(f); });
			std::wstring q = L"SELECT " + PVX::String::Join(primitive, L",", [](const std::wstring & f) { return L"[" + f + L"]"; });
			if (json.size()) q += (primitive.size() ? L"," : L"") + PVX::String::Join(json, L",", [](const std::wstring & f) { return L"JSON_QUERY([" + f + L"], '$') [" + f + L"]"; });
			q += L"\nFROM " + Table;
			if (Where.size()) q += L"\nWHERE " + Where;
			return StringSelect(q);
		}

		std::wstring SQL::StringSelect(const std::wstring & query, const std::vector<ParamTuple>& params) {
			auto r = Query(L"SELECT (" + query + L"\nFOR JSON AUTO) json", params);
			if (r.Read()) return r[0].WCharArray;
			return L"[]";
		}

		//PVX::JSON::Item SQL::JsonUpsert(const std::wstring & Table, const std::vector<ParamTuple>& Values, const std::set<std::wstring>& ConditionParams, bool InsertKeys) {
		//	auto IdItems = PVX::Filter(Values, [&ConditionParams](const ParamTuple& p) { return ConditionParams.count(p.Name); });
		//	if (Exist(Table, IdItems)) {
		//		Update(Table, Values, ConditionParams);
		//		PVX::JSON::Item ret = PVX::JSON::jsElementType::Object;
		//		for (auto & i : IdItems) {
		//			auto & it = ret[i.Name];
		//			switch (i.Param.cType) {
		//				case SQL_C_DOUBLE: it = (*(Ref*)&i.Param.Data[0]).Double; break;
		//			case SQL_C_SBIGINT: it = (*(Ref*)&i.Param.Data[0]).Long; break;
		//			case SQL_C_WCHAR:
		//				it.Type = PVX::JSON::jsElementType::String;
		//				it.String.resize(i.Param.Data.size() >> 1);
		//				memcpy(&it.String[0], i.Param.Data.data(), i.Param.Data.size());
		//				break;
		//			case SQL_C_CHAR:
		//				it.Type = PVX::JSON::jsElementType::String;
		//				it.String.resize(i.Param.Data.size());
		//				memcpy(&it.String[0], i.Param.Data.data(), i.Param.Data.size());
		//				break;
		//			}
		//		}
		//		return PVX::JSON::jsArray({ ret });
		//	} else {
		//		if (InsertKeys) return JsonInsert(Table, Values);
		//		return JsonInsert(Table, PVX::Filter(Values, [&ConditionParams](const ParamTuple& p) { return !ConditionParams.count(p.Name); }));
		//	}
		//}

		PVX::JSON::Item SQL::ReaderToJson(SQL_Reader reader) {
			PVX::JSON::Item ret = PVX::JSON::jsElementType::Array;
			while (reader.Read()) {
				PVX::JSON::Item line = PVX::JSON::jsElementType::Object;
				std::wstring ws;
				std::string s;
				for (auto & f : reader.FieldByNum) {
					if (!f.IsNull) {
						switch (f.DataType) {
						case SQL_UNICODE_VARCHAR:
							line[f.ColumnName] = (*(Ref*)&f.Data[0]).WCharArray;
							break;
						case SQL_UNICODE_LONGVARCHAR:
							ws = (*(Ref*)&f.Data[0]).WCharArray;
							if (ws.size() > f.Data.size() >> 1) ws.resize(f.Data.size() >> 1);
							line[f.ColumnName] = ws;
							break;
						case SQL_VARCHAR:
							line[f.ColumnName] = (*(Ref*)&f.Data[0]).CharArray;
							break;
						case SQL_LONGVARCHAR:
							s = (*(Ref*)&f.Data[0]).CharArray;
							if (s.size() > f.Data.size()) s.resize(f.Data.size());
							line[f.ColumnName] = s;
							break;
						case SQL_FLOAT:
						case SQL_DOUBLE:
						case SQL_NUMERIC:
							line[f.ColumnName] = (*(Ref*)&f.Data[0]).Double;
							break;
						case SQL_INTEGER:
							line[f.ColumnName] = (*(Ref*)&f.Data[0]).Int;
							break;
						case SQL_BIGINT:
							line[f.ColumnName] = (*(Ref*)&f.Data[0]).Long;
							break;
						case SQL_BIT:
							line[f.ColumnName] = (*(Ref*)&f.Data[0]).Char;
							break;
						default:
							throw std::exception((VarTypeMap[f.DataType] + " Unimplemented").c_str());
						}
					}
				}
				ret.push(line);
			}
			return ret;
		}

		std::vector<std::wstring> SQL::GetTables() {
			auto reader = Query(L"SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE = 'BASE TABLE'");
			std::vector<std::wstring> ret;
			while (reader.Read()) {
				ret.push_back(std::wstring(reader[0].WCharArray));
			}
			return ret;
		}

		std::unordered_map<std::wstring, std::wstring> SQL::GetTableFields(const std::wstring& table) {
			auto reader = Query(L"SELECT COLUMN_NAME, DATA_TYPE FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = @table", {
				{ L"table", table }
			});
			std::unordered_map<std::wstring, std::wstring> ret;
			while (reader.Read()) {
				auto test = reader[0].WCharArray;
				ret[test] = reader[1].WCharArray;
			}
			return ret;
		}

		std::unordered_map<std::wstring, std::wstring> SQL::GetTableKeys(const std::wstring& table) {
			return std::unordered_map<std::wstring, std::wstring>();
		}

		PVX::JSON::Item SQL::JsonQuery(const std::wstring & query, const std::vector<ParamTuple> & params) {
			return ReaderToJson(Query(query, params));
		}

		PVX::JSON::Item SQL::JsonQueryJson(const std::wstring & query, const PVX::JSON::Item & params) {
			std::vector<ParamTuple> Params;
			for (auto &[Name, Value] : params.getObject()) {
				Params.push_back({ Name, Value });
			}
			return JsonQuery(query, Params);
		}

		const int SQL_Reader::FieldCount() {
			return Names.size();
		}
		const std::wstring & SQL_Reader::FieldName(int Index) {
			return Names[Index];
		}
		const int SQL_Reader::FieldType(int Index) {
			return FieldByNum[Index].DataType;
		}
		const int SQL_Reader::FieldType(const std::wstring & Name) {
			return Fields[Name]->DataType;
		}
		Ref & SQL_Reader::operator[](int Index) {
			auto & f = FieldByNum[Index];
			if (!f.Read) ReadColumn(f);
			return *(Ref*)f.Data.data();
		}
		Ref & SQL_Reader::operator[](const std::wstring & Name) {
			auto & f = *Fields[Name];
			if (!f.Read) ReadColumn(f);
			return *(Ref*)&f.Data[0];
		}
		std::vector<unsigned char> & SQL_Reader::operator()(int Index) {
			auto & f = FieldByNum[Index];
			if (!f.Read) ReadColumn(f);
			return f.Data;
		}
		std::vector<unsigned char> & SQL_Reader::operator()(const std::wstring & Name) {
			auto & f = *Fields[Name];
			if (!f.Read) ReadColumn(f);
			return f.Data;
		}
		int SQL_Reader::IsNull(int Index) {
			auto & f = FieldByNum[Index];
			if (!f.Read) ReadColumn(f);
			return f.IsNull;
		}
		int SQL_Reader::IsEmpty(int Index) {
			auto & f = FieldByNum[Index];
			if (!f.Read) ReadColumn(f);
			return !f.Data.size();
		}
		int SQL_Reader::IsNullOrEmpty(int Index) {
			auto & f = FieldByNum[Index];
			if (!f.Read) ReadColumn(f);
			return f.IsNull || !f.Data.size();
		}
		int SQL_Reader::IsNull(const std::wstring & Name) {
			auto & f = *Fields[Name];
			if (!f.Read) ReadColumn(f);
			return f.IsNull;
		}
		int SQL_Reader::IsEmpty(const std::wstring & Name) {
			auto & f = *Fields[Name];
			if (!f.Read) ReadColumn(f);
			return !f.Data.size();
		}
		int SQL_Reader::IsNullOrEmpty(const std::wstring & Name) {
			auto & f = *Fields[Name];
			if (!f.Read) ReadColumn(f);
			return f.IsNull || !f.Data.size();
		}

		const std::vector<std::wstring>& SQL_Reader::GetNames() {
			return Names;
		}
		Ref & SQL_Reader::ReadColumn(int Index) {
			return *(Ref*)&ReadColumn(FieldByNum[Index])[0];
		}
		Ref & SQL_Reader::ReadColumn(const std::wstring & Name) {
			return *(Ref*)&ReadColumn(*Fields[Name])[0];
		}
		std::vector<unsigned char> SQL_Reader::ReadColumnData(int Index) {
			return ReadColumn(FieldByNum[Index]);
		}
		std::vector<unsigned char> SQL_Reader::ReadColumnData(const std::wstring & Name) {
			return ReadColumn(*Fields[Name]);
		}

		std::vector<unsigned char> & SQL_Reader::ReadColumn(Field & f) {
			SQLLEN sz;
			sz = f.Size;
			f.IsNull = 0;
			f.Read = 1;
			if (f.Size) {
				f.Data.resize(f.Size);
				auto test = SQLGetData(hStmt, f.ColumnNumber, VarTypeMapToC[f.DataType], &f.Data[0], f.Data.size(), &sz);
				if (sz < 0) {
					f.IsNull = 1;
					f.Data.resize(0);
				} else if (f.DataType == SQL_NUMERIC) {
					*(double*)&f.Data[0] = (*(long*)(&f.Data[3])) * pow(10.0, -f.Data[1]);
					f.Data.resize(8);
				} else {
					f.Data.resize(sz);
				}
			} else {
				char zero;
				if (SQLGetData(hStmt, f.ColumnNumber, SQL_C_BINARY, &zero, 0, &sz)) {
					f.Data.resize(sz);
					SQLGetData(hStmt, f.ColumnNumber, SQL_C_BINARY, &f.Data[0], sz, &sz);
				} else {
					f.IsNull = 1;
					f.Data.resize(0);
				}
			}
			if (f.Data.size() && (f.DataType == SQL_UNICODE_VARCHAR || f.DataType == SQL_UNICODE_LONGVARCHAR)) {
				f.Data.push_back(0);
				f.Data.push_back(0);
			}
			return f.Data;
		}

		int SQL_Reader::Read() {
			auto err = SQLFetch(hStmt);
			if (err == SQL_SUCCESS) {
				for (auto & f : FieldByNum) f.Read = 0;
				//for (auto i = 0; i < Names.size(); i++) {
				//	ReadColumn(FieldByNum[i]);
				//}
				return 1;
			}
			Error = GetError(hStmt);
			return 0;
		}
		SQL_Reader::SQL_Reader(const SQLHSTMT & hStmt) : hStmt(hStmt) {
			SQLSMALLINT sNumResults;
			SQLNumResultCols(hStmt, &sNumResults);

			SQLCHAR ColumnName[1024];
			SQLSMALLINT BufferLength;
			SQLSMALLINT	NameLength;
			SQLSMALLINT DataType;
			SQLULEN		ColumnSize;
			SQLSMALLINT	DecimalDigits;
			SQLSMALLINT	Nullable;
			SQLRETURN err;
			SQLLEN	indPtr;

			long long	FieldSize, ssType;

			for (SQLSMALLINT i = 1; i <= sNumResults; i++) {
				SQLDescribeColA(hStmt, i, ColumnName, 1024, &NameLength, &DataType, &ColumnSize, &DecimalDigits, &Nullable);
				short sz = (short)ColumnSize;
				auto colName = PVX::Decode::UTF(ColumnName, NameLength);

				SQLColAttribute(hStmt, i, SQL_DESC_OCTET_LENGTH, NULL, 0, NULL, (SQLLEN*)&FieldSize);
				SQLColAttribute(hStmt, i, SQL_DESC_TYPE, NULL, 0, NULL, (SQLLEN*)&ssType);

				std::string tp = VarTypeMap[ssType];

				Names.push_back(colName);
				FieldByNum.push_back({
					colName,
					std::vector<unsigned char>(),
					sz > 0 ? FieldSize : 0,
					DataType,
					DecimalDigits,
					Nullable,
					i,
					0
				});
			}
			for (auto & f : FieldByNum) Fields[f.ColumnName] = &f;
		}
		Parameter::Parameter(const int & Value) : Data(8) {
			IsBig = 0;
			*(long long*)&Data[0] = Value;
			cType = SQL_C_SBIGINT;
			Type = SQL_BIGINT;
			ColumnSize = BufferSize = len = Data.size();
		}
		Parameter::Parameter(const float & Value) : Data(8) {
			IsBig = 0;
			*(double*)&Data[0] = Value;
			cType = SQL_C_DOUBLE;
			Type = SQL_FLOAT;
			ColumnSize = BufferSize = len = Data.size();
		}
		Parameter::Parameter(const double & Value) : Data(8) {
			IsBig = 0;
			*(double*)&Data[0] = Value;
			cType = SQL_C_DOUBLE;
			Type = SQL_FLOAT;
			ColumnSize = BufferSize = len = Data.size();
		}
		Parameter::Parameter(const long long & Value) : Data(8) {
			IsBig = 0;
			*(long long*)&Data[0] = Value;
			cType = SQL_C_SBIGINT;
			Type = SQL_BIGINT;
			ColumnSize = BufferSize = len = Data.size();
		}
		Parameter::Parameter(const std::string & Value) : Data(Value.size()) {
			IsBig = 0;
			if (Data.size()) memcpy(&Data[0], Value.c_str(), Data.size());
			cType = SQL_C_CHAR;
			Type = SQL_VARCHAR;
			ColumnSize = BufferSize = len = Data.size();
		}
		Parameter::Parameter(const std::wstring & Value) : Data(Value.size() << 1) {
			IsBig = 0;
			if (Data.size()) memcpy(&Data[0], Value.c_str(), Data.size());
			cType = SQL_C_WCHAR;
			Type = SQL_UNICODE_VARCHAR;
			ColumnSize = BufferSize = len = Data.size();
		}
		Parameter::Parameter(const wchar_t * Value) {
			IsBig = 0;
			std::wstring tmp = Value;
			Data.resize(tmp.size() << 1);
			if (Data.size()) memcpy(&Data[0], tmp.c_str(), Data.size());
			cType = SQL_C_WCHAR;
			Type = SQL_UNICODE_VARCHAR;
			ColumnSize = BufferSize = len = Data.size();
		}
		Parameter::Parameter(const bool Value) :Data(4) {
			IsBig = 0;
			Data[0] = Value;
			cType = SQL_C_BIT;
			Type = SQL_BIT;
			ColumnSize = BufferSize = len = Data.size();
		}
		Parameter::Parameter(const std::vector<unsigned char> & Value) {
			IsBig = 0;
			Data = Value;
			cType = SQL_C_BINARY;
			Type = SQL_BINARY;
			ColumnSize = BufferSize = len = Data.size();
		}

		Parameter::Parameter(const BigParameter & p) {
			IsBig = 1;
			Data = p.Data;
			cType = p.cType;
			Type = p.Type;
			len = SQL_LEN_DATA_AT_EXEC(p.len);
			BufferSize = p.len;
			ColumnSize = 2147483647;
		}

		Parameter::Parameter(const NullParameter & p) : Data(1) {
			IsBig = 0;
			cType = SQL_C_BINARY;
			Type = SQL_VARCHAR;
			ColumnSize = 1;
			BufferSize = 1;
			len = SQL_NULL_DATA;
		}

		Parameter::Parameter(const PVX::JSON::Item & p) {
			switch (p.Type()) {
			case PVX::JSON::jsElementType::Null:
			case PVX::JSON::jsElementType::Undefined: (*this) = NullParameter(); break;
			case PVX::JSON::jsElementType::Float: (*this) = p.Double(); break;
			case PVX::JSON::jsElementType::Integer: (*this) = p.Integer(); break;
			case PVX::JSON::jsElementType::String: (*this) = p.String(); break;
			case PVX::JSON::jsElementType::Boolean: (*this) = (bool)p.Boolean(); break;
			case PVX::JSON::jsElementType::Object:
			case PVX::JSON::jsElementType::Array:
				(*this) = PVX::JSON::stringify(p); break;
			}
		}

		BigParameter::BigParameter(const std::string & Value) : Data(Value.size()) {
			memcpy(&Data[0], Value.c_str(), Data.size());
			cType = SQL_C_CHAR;
			Type = SQL_LONGVARCHAR;
			len = Data.size();
		}
		BigParameter::BigParameter(const std::wstring & Value) : Data(Value.size() << 1) {
			memcpy(&Data[0], Value.c_str(), Data.size());
			cType = SQL_C_WCHAR;
			Type = SQL_UNICODE_LONGVARCHAR;
			len = Data.size();
		}
		BigParameter::BigParameter(const std::vector<unsigned char> & Value) {
			Data = Value;
			cType = SQL_C_BINARY; // SQL_C_CHAR; // 
			Type = SQL_LONGVARBINARY;
			len = Data.size();
		}
		std::vector<std::wstring> SQL::Tables() {
			std::vector<std::wstring> ret;
			auto r = Query(L"SELECT TABLE_NAME t FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE='BASE TABLE'");
			while (r.Read()) {
				ret.push_back(r.ReadColumn(0).WCharArray);
			}
			return ret;
		}
		PVX::JSON::Item SQL::Columns(const std::wstring & Table) {
			return PVX::JSON::parse(stringQuery(LR"raw(SELECT s.COLUMN_NAME 'Column', s.DATA_TYPE 'Type', IsNumeric(k.ORDINAL_POSITION) IsKey, case when s.IS_NULLABLE='YES' then 1 else 0 end 'IsNull'
FROM INFORMATION_SCHEMA.COLUMNS s
left join INFORMATION_SCHEMA.KEY_COLUMN_USAGE k
on s.TABLE_NAME=k.TABLE_NAME and s.COLUMN_NAME=k.COLUMN_NAME and OBJECTPROPERTY(OBJECT_ID(k.CONSTRAINT_SCHEMA + '.' + QUOTENAME(k.CONSTRAINT_NAME)), 'IsPrimaryKey') <> 0
WHERE s.TABLE_NAME = @table
order by s.ORDINAL_POSITION
)raw", { { { L"table", Table } } })).DeepReducedCopy();
		}
		PVX::JSON::Item SQL::ColumnsWithKeys(const std::wstring & Table) {
			return PVX::JSON::parse(stringQuery(LR"raw(SELECT s.COLUMN_NAME 'Column', s.DATA_TYPE 'Type', IsNumeric(k.ORDINAL_POSITION) IsKey, case when s.IS_NULLABLE='YES' then 1 else 0 end 'IsNull', ForeignKeyTo.TABLE_NAME 'Table', ForeignKeyTo.COLUMN_NAME 'Column'
FROM INFORMATION_SCHEMA.COLUMNS s
left join INFORMATION_SCHEMA.KEY_COLUMN_USAGE k
on s.TABLE_NAME=k.TABLE_NAME and s.COLUMN_NAME=k.COLUMN_NAME and OBJECTPROPERTY(OBJECT_ID(k.CONSTRAINT_SCHEMA + '.' + QUOTENAME(k.CONSTRAINT_NAME)), 'IsPrimaryKey') <> 0
left join INFORMATION_SCHEMA.KEY_COLUMN_USAGE f
on s.TABLE_NAME=f.TABLE_NAME and s.COLUMN_NAME=f.COLUMN_NAME and OBJECTPROPERTY(OBJECT_ID(f.CONSTRAINT_SCHEMA + '.' + QUOTENAME(f.CONSTRAINT_NAME)), 'IsForeignKey') <> 0
left join INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS tt
on f.CONSTRAINT_NAME=tt.CONSTRAINT_NAME
left join INFORMATION_SCHEMA.KEY_COLUMN_USAGE ForeignKeyTo
on tt.UNIQUE_CONSTRAINT_NAME=ForeignKeyTo.CONSTRAINT_NAME
WHERE s.TABLE_NAME = @table
order by s.ORDINAL_POSITION
)raw", { { { L"table", Table } } })).DeepReducedCopy();
		}
	}
}