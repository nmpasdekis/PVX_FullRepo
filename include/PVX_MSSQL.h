#include<Windows.h>
#include<sql.h>
#include<sqlext.h>
#include<string>
#include<vector>
#include<map>
#include<set>
#include<initializer_list>
#include <PVX_json.h>
#include <unordered_map>

// "DRIVER={SQL Server};SERVER=(local)\\SQLEXPRESS;DATABASE=PerVertEX;UID=sa;PWD=16777215;"

namespace PVX {
	namespace DB {
		typedef union Ref {
			char				Char, CharArray[1];
			unsigned char		UChar, UCharArray[1];
			wchar_t				WChar, WCharArray[1];
			short				Short, ShortArray[1];
			unsigned short		UShort, UShortArray[1];
			int					Int, IntArray[1];
			unsigned int		UInt, UIntArray[1];
			int64_t			Long, LongArray[1];
			uint64_t	ULong, ULongArray[1];


			float Float;
			float FloatArray[1];
			double Double;
			double DoubleArray[1];
			void* PointerArray[1];
		} Ref;

		class BigParameter {
		public:
			BigParameter() {};
			BigParameter(const std::string& Value);
			BigParameter(const std::wstring& Value);
			BigParameter(const std::vector<unsigned char>& Value);
		protected:
			std::vector<unsigned char> Data;
			int cType{}, Type{};
			SQLLEN len{};
			friend class Parameter;
		};

		class NullParameter {
		public:
			NullParameter() {};
		};

		class Parameter {
		public:
			Parameter() {};
			Parameter(const bool Value);
			Parameter(const int& Value);
			Parameter(const float& Value);
			Parameter(const double& Value);
			Parameter(const int64_t& Value);
			Parameter(const std::string& Value);
			Parameter(const std::wstring& Value);
			Parameter(const wchar_t* Value);
			Parameter(const std::vector<unsigned char>& Value);
			Parameter(const BigParameter& p);
			Parameter(const NullParameter& p);
			Parameter(const PVX::JSON::Item& p);
		protected:
			std::vector<unsigned char> Data;
			int cType{}, Type{}, IsBig{};
			SQLULEN ColumnSize{};
			SQLLEN BufferSize{};
			SQLLEN len{};
			friend class SQL;
		};

		class ParamTuple {
		public:
			std::wstring Name;
			Parameter Param;
		};

		class SQL_Reader {
		protected:
			struct Field {
				std::wstring	ColumnName;
				std::vector<unsigned char> Data;
				int64_t		Size;
				SQLSMALLINT		DataType;
				SQLSMALLINT		DecimalDigits;
				SQLSMALLINT		Nullable;
				SQLSMALLINT		ColumnNumber;
				int IsNull, Read;
			};
		public:
			int Read();
			const size_t FieldCount() const;
			const std::wstring& FieldName(int Index) const;
			const int FieldType(int Index) const;
			const int FieldType(const std::wstring& Name) const;
			const Ref& operator[](int Index);
			const Ref& operator[](const std::wstring& Name);
			const std::vector<unsigned char>& operator()(int Index);
			const std::vector<unsigned char>& operator()(const std::wstring& Name);
			bool IsNull(int Index);
			bool IsEmpty(int Index);
			bool IsNullOrEmpty(int Index);
			bool IsNull(const std::wstring& Name);
			bool IsEmpty(const std::wstring& Name);
			bool IsNullOrEmpty(const std::wstring& Name);
			const std::vector<std::wstring>& GetNames();
			Ref& ReadColumn(int Index);
			Ref& ReadColumn(const std::wstring& Name);
			std::vector<unsigned char> ReadColumnData(int Index);
			std::vector<unsigned char> ReadColumnData(const std::wstring& Name);

			~SQL_Reader() {
				SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
			}
		protected:
			friend class SQL;
			SQLHSTMT hStmt;
			std::string Error;
			std::vector<unsigned char>& ReadColumn(Field& f);
			std::map<std::wstring, Field*> Fields;
			std::vector<Field> FieldByNum;
			std::vector<std::wstring> Names;
			SQL_Reader(const SQLHSTMT& hStmt);
			SQL_Reader(const std::wstring& Error);
		};

		class SQL {
		public:
			SQL();
			SQL(const std::string& connectionString);
			SQL(const std::string& Database, const std::string& Username, const std::string& Password, const std::string& Server = "(local)");
			~SQL();

			inline static std::string MakeConnectionString(const std::string& Database, const std::string& Username, const std::string& Password, const std::string& Server = "(local)") {
				return "DRIVER={SQL Server};SERVER=" + Server + ";DATABASE=" + Database + ";UID=" + Username + ";PWD=" + Password + ";";
			}

			static void SetConnectionString(const std::string& Connection);
			static void SetConnectionString(const std::string& Database, const std::string& Username, const std::string& Password, const std::string& Server = "(local)\\SQLEXPRESS");

			std::vector<std::wstring> Tables();
			PVX::JSON::Item Columns(const std::wstring& Table);
			PVX::JSON::Item ColumnsWithKeys(const std::wstring& Table);

			SQL_Reader Query(const std::wstring& query, const std::vector<ParamTuple>& params = {});
			SQLRETURN Execute(const std::wstring& query, const std::vector<ParamTuple>& params = {});
			PVX::JSON::Item JsonQuery(const std::wstring& query, const std::vector<ParamTuple>& params = {});
			PVX::JSON::Item JsonQueryJson(const std::wstring& query, const PVX::JSON::Item& params);
			PVX::JSON::Item JsonQueryJson2(const std::wstring& query, const PVX::JSON::Item& params);

			std::wstring stringQuery(const std::wstring& query, const std::vector<ParamTuple>& params = {});
			std::wstring stringQueryJson(const std::wstring& query, const PVX::JSON::Item& params);

			int64_t Exist(const std::wstring& Table, const std::vector<ParamTuple>& Values, const std::wstring& Where = L"");
			int64_t ExistJson(const std::wstring& Table, const PVX::JSON::Item& Values, const std::wstring& Where = L"");

			SQL_Reader Update(const std::wstring& Table, const std::vector<ParamTuple>& Values, const std::set<std::wstring>& ConditionParams, const std::wstring& Where = L"", const std::vector<std::wstring>& ReturnFields = {});
			SQL_Reader UpdateJson(const std::wstring& Table, const PVX::JSON::Item& Values, const std::set<std::wstring>& ConditionParams, const std::wstring& Where = L"", const std::vector<std::wstring>& ReturnFields = {});
			PVX::JSON::Item JsonUpdate(const std::wstring& Table, const std::vector<ParamTuple>& Values, const std::set<std::wstring>& ConditionParams, const std::wstring& Where = L"", const std::vector<std::wstring>& ReturnFields = {});
			PVX::JSON::Item JsonUpdateJson(const std::wstring& Table, const PVX::JSON::Item& Values, const std::set<std::wstring>& ConditionParams, const std::wstring& Where = L"", const std::vector<std::wstring>& ReturnFields = {});

			SQL_Reader Delete(const std::wstring& Table, const std::vector<ParamTuple>& Params, const std::wstring& Where = L"", const std::vector<std::wstring>& ReturnFields = {});
			SQL_Reader DeleteJson(const std::wstring& Table, const PVX::JSON::Item& Params, const std::wstring& Where = L"", const std::vector<std::wstring>& ReturnFields = {});
			PVX::JSON::Item JsonDelete(const std::wstring& Table, const std::vector<ParamTuple>& Params, const std::wstring& Where = L"", const std::vector<std::wstring>& ReturnFields = {});
			PVX::JSON::Item JsonDeleteJson(const std::wstring& Table, const PVX::JSON::Item& Params, const std::wstring& Where = L"", const std::vector<std::wstring>& ReturnFields = {});

			SQL_Reader Insert(const std::wstring& Table, const std::vector<ParamTuple>& Params, const std::vector<std::wstring>& ReturnFields = {});
			SQL_Reader InsertJson(const std::wstring& Table, const PVX::JSON::Item& Params, const std::vector<std::wstring>& ReturnFields = {});
			PVX::JSON::Item JsonInsert(const std::wstring& Table, const std::vector<ParamTuple>& Params, const std::vector<std::wstring>& ReturnFields = {});
			PVX::JSON::Item JsonInsertJson(const std::wstring& Table, const PVX::JSON::Item& Params, const std::vector<std::wstring>& ReturnFields = {});


			//PVX::JSON::Item JsonUpsertJson(const std::wstring& Table, const PVX::JSON::Item & Params, const std::set<std::wstring> & ConditionParams, bool InsertKeys = false);
			//PVX::JSON::Item JsonUpsert(const std::wstring& Table, const std::vector<ParamTuple> & Values, const std::set<std::wstring> & ConditionParams, bool InsertKeys = false);

			PVX::JSON::Item SelectJson(const std::wstring& Table, const std::vector<std::wstring>& Fields, const std::set<std::wstring>& JsonFields = {}, const std::wstring& Where = L"");
			PVX::JSON::Item SelectJson(const std::wstring& query, const std::vector<ParamTuple>& params = {});
			std::wstring StringSelectJson(const std::wstring& Table, const std::vector<std::wstring>& Fields, const std::set<std::wstring>& JsonFields = {}, const std::wstring& Where = L"");
			std::wstring StringSelect(const std::wstring& query, const std::vector<ParamTuple>& params = {});

			static PVX::JSON::Item ReaderToJson(SQL_Reader reader);

			std::vector<std::wstring> GetTables();
			std::unordered_map<std::wstring, std::wstring> GetTableFields(const std::wstring& table);
			std::unordered_map<std::wstring, std::wstring> GetTableKeys(const std::wstring& table);
		private:
			std::string Error;
			static std::string DefaultConnectionString;
			SQLRETURN Result{};
			SQLHSTMT Exec(const std::wstring& query, const std::vector<ParamTuple>& params = {});
			SQLHENV     hEnv{};
			SQLHDBC     hDbc{};
			//std::vector<SQLHSTMT> Statements;
		};
	}
}