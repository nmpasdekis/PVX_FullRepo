#ifndef __PVX_JSON_H__
#define __PVX_JSON_H__

#include <string>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
#include <initializer_list>
#include <functional>
#include <PVX_Encode.h>
#include <PVX_String.h>
#include <variant>

namespace PVX {
	namespace JSON {
		typedef enum class jsElementType: unsigned char {
			Undefined,
			Null,
			Boolean,
			Integer,
			Float,
			String,
			Array,
			Object,
			Binary
		} jsElementType;

		enum class BSON_Type : unsigned char {
			Double = 0x01,
			String = 0x02,
			Object = 0x03,
			Array = 0x04,
			Binary = 0x05,
			Undefined = 0x06,
			ObjectId = 0x07,
			Boolean = 0x08,
			DateTimeUTC = 0x09,
			Null = 0x0A,
			Regex = 0x0B,
			DBPointer = 0x0C,
			Code = 0x0D,
			Symbol = 0x0E,
			Code_w_s = 0x0F,
			Int32 = 0x10,
			Timestamp = 0x11,
			Int64 = 0x12,
			Decimal128 = 0x13,
			MinKey = 0xFF,
			MaxKey = 0x7F
		};

		class jsArray;
		class Item;

		struct Variant_String {
			int refCount;
			std::wstring String;
		};
		struct Variant_Array {
			int refCount;
			std::vector<Item> Array;
		};
		struct Variant_Object {
			int refCount;
			std::unordered_map<std::wstring, Item> Object;
		};

		struct Variant_BSON_Binary {
			int refCount;
			int Type;
			std::vector<unsigned char> Bytes;
		};

		struct Variant {
			jsElementType GetType() const { return Type; }

			~Variant() { Release(); }
			Variant() : Type{ jsElementType::Undefined }, BsonType{ BSON_Type::Undefined } { Value.Integer = 0; }
			Variant(const int v) : Type{ jsElementType::Integer }, BsonType{ BSON_Type::Int32 } { Value.Integer = v; }
			Variant(const long long v) : Type{ jsElementType::Integer }, BsonType{ BSON_Type::Int64 } { Value.Integer = v; }
			Variant(const double v) : Type{ jsElementType::Float }, BsonType{ BSON_Type::Double } { Value.Double = v; }
			Variant(const bool v) : Type{ jsElementType::Boolean }, BsonType{ BSON_Type::Boolean } { Value.Boolean = v; }
			Variant(const nullptr_t v) : Type{ jsElementType::Null }, BsonType{ BSON_Type::Null } { Value.Integer = 0; }
			Variant(const wchar_t* v) : Type{ jsElementType::String }, BsonType{ BSON_Type::String } { Value.String = new Variant_String{ 1, v }; }
			Variant(const std::wstring& v) : Type{ jsElementType::String }, BsonType{ BSON_Type::String } { Value.String = new Variant_String{ 1, v }; }
			Variant(const std::vector<Item>& v) : Type{ jsElementType::Array }, BsonType{ BSON_Type::Array } { Value.Array = new Variant_Array{ 1, v }; }
			Variant(const std::unordered_map<std::wstring, Item>& v) : Type{ jsElementType::Object }, BsonType{ BSON_Type::Object } { Value.Object = new Variant_Object{ 1, v }; }
			Variant(const std::vector<unsigned char>& v):Type{ jsElementType::Binary }, BsonType{ BSON_Type::Binary } { Value.Binary = new Variant_BSON_Binary{ 1, 0, v }; }
			Variant(const jsElementType tp) : Type{ tp } {
				switch (tp) {
					case jsElementType::Undefined: Value.Integer = 0ll; BsonType = BSON_Type::Undefined; break;
					case jsElementType::Null: Value.Integer = 0ll; BsonType = BSON_Type::Null; break;
					case jsElementType::Boolean: Value.Integer = 0ll; BsonType = BSON_Type::Boolean; break;
					case jsElementType::Integer: Value.Integer = 0ll; BsonType = BSON_Type::Int64; break;
					case jsElementType::Float: Value.Double = 0.0; BsonType = BSON_Type::Double; break;
					case jsElementType::String: Value.String = new Variant_String{ 1 }; BsonType = BSON_Type::String; break;
					case jsElementType::Array: Value.Array = new Variant_Array{ 1 }; BsonType = BSON_Type::Array; break;
					case jsElementType::Object: Value.Object = new Variant_Object{ 1 }; BsonType = BSON_Type::Object; break;
					case jsElementType::Binary: Value.Binary = new Variant_BSON_Binary{ 1 }; BsonType = BSON_Type::Binary; break;
					default: Value.Boolean = false; BsonType = BSON_Type::Undefined; break;
				}
			}
			Variant(const Variant& v) :Value{ v.Value }, Type{ v.Type }, BsonType{ v.BsonType } {
				if (Type>=jsElementType::String) Value.String->refCount++;
			}
			Variant(Variant&& v) noexcept :Value{ v.Value }, Type{ v.Type }, BsonType{ v.BsonType } {
				v.Type = PVX::JSON::jsElementType::Undefined;
			}


			bool& operator=(const bool v) { Release(); Type = jsElementType::Boolean; Value.Boolean = v; BsonType = BSON_Type::Boolean; return Value.Boolean; }
			long long& operator=(const int v) { Release(); Type = jsElementType::Integer; Value.Integer = v; BsonType = BSON_Type::Int64; return Value.Integer; }
			long long& operator=(const long long v) { Release(); Type = jsElementType::Integer; Value.Integer = v; BsonType = BSON_Type::Int64; return Value.Integer; }
			double& operator=(const float v) { Release(); Type = jsElementType::Float; Value.Double = v; BsonType = BSON_Type::Double; return Value.Double; }
			double& operator=(const double v) { Release(); Type = jsElementType::Float; Value.Double = v; BsonType = BSON_Type::Double; return Value.Double; }

			nullptr_t operator=(const nullptr_t& t) {
				Release();
				Value.Integer = 0;
				Type = jsElementType::Null;
				BsonType = BSON_Type::Null;
				return nullptr;
			}
			std::wstring& operator=(const wchar_t* v) {
				Release();
				Type = jsElementType::String;
				BsonType = BSON_Type::String;
				Value.String = new Variant_String{ 1, v };
				return Value.String->String;
			}
			std::wstring& operator=(const std::wstring& v) {
				Release();
				Type = jsElementType::String;
				BsonType = BSON_Type::String;
				Value.String = new Variant_String{ 1, v };
				return Value.String->String;
			}
			std::vector<unsigned char>& operator=(const std::vector<unsigned char>& v) {
				Release();
				Type = jsElementType::Binary;
				BsonType = BSON_Type::Binary;
				Value.Binary = new Variant_BSON_Binary{ 1, 0, v };
				return Value.Binary->Bytes;
			}

			std::vector<Item>& operator=(const std::vector<Item>& v) { Release(); Type = jsElementType::Array; BsonType = BSON_Type::Array; Value.Array = new Variant_Array{ 1, v }; return Value.Array->Array; }
			std::unordered_map<std::wstring, Item>& operator=(const std::unordered_map<std::wstring, Item>& v) { Release(); Type = jsElementType::Object; BsonType = BSON_Type::Object; Value.Object = new Variant_Object{ 1, v }; return Value.Object->Object; }


			Variant& operator=(const Variant& v) {
				if (v.Type>=jsElementType::String) v.Value.String->refCount++;
				Release();
				Type = v.Type;
				BsonType = v.BsonType;
				Value = v.Value;
				return *this;
			}

			operator int() { return Value.Integer; }
			operator long long() { return Value.Integer; }
			operator float() { return Value.Double; }
			operator double() { return Value.Double; }
			operator bool() { return Value.Boolean; }

			long long& Integer() { return Value.Integer; }
			double& Double() { return Value.Double; }
			bool& Boolean() { return Value.Boolean; }

			long long Integer() const { return Value.Integer; }
			double Double() const { return Value.Double; }
			bool Boolean() const { return Value.Boolean; }

			std::wstring& String() { return Value.String->String; }
			std::vector<Item>& Array() { return Value.Array->Array; }
			std::unordered_map<std::wstring, Item>& Object() { return Value.Object->Object; }
			std::vector<unsigned char>& Binary() { return Value.Binary->Bytes; }

			const std::wstring& String() const { return Value.String->String; }
			const std::vector<Item>& Array() const { return Value.Array->Array; }
			const std::unordered_map<std::wstring, Item>& Object() const { return Value.Object->Object; }
			const std::vector<unsigned char>& Binary() const { return Value.Binary->Bytes; }

			BSON_Type& Bson() { return BsonType; }
			const BSON_Type& Bson() const { return BsonType; }
		private:
			BSON_Type BsonType;
			jsElementType Type;
			unsigned short Padding;
			union {
				bool Boolean;
				long long Integer;
				double Double;
				Variant_String* String;
				Variant_Array* Array;
				Variant_Object* Object;
				Variant_BSON_Binary* Binary;
			} Value;

			void Release() {
				if (Type!=PVX::JSON::jsElementType::Undefined) {
					switch (Type) {
						case PVX::JSON::jsElementType::String: if (!--Value.String->refCount)
							delete Value.String;
							break;
						case PVX::JSON::jsElementType::Array: if (!--Value.Array->refCount)
							delete Value.Array;
							break;
						case PVX::JSON::jsElementType::Object: if (!--Value.Object->refCount)
							delete Value.Object;
							break;
						case PVX::JSON::jsElementType::Binary: if (!--Value.Object->refCount)
							delete Value.Binary;
							break;
					}
					Type = jsElementType::Undefined;
					BsonType = BSON_Type::Undefined;
				}
			}
			friend class Item;
		};

		class Item {
		public:
			Item() {}
			Item(const Item&) = default;
			Item(Item&&) = default;

			Item(const enum jsElementType& tp) : Value{ tp } {}
			Item(const bool& v) : Value{ v } {}
			Item(const int& v) : Value{ v } {}
			Item(const long long& v) : Value{ v } {}
			Item(const unsigned long long& v) : Value{ long long(v) } {}
			Item(const float& v) : Value{ v } {}
			Item(const double& v) : Value{ v } {}
			Item(const std::wstring& s) : Value{ s } { ; }
			Item(const wchar_t* s) : Value{ s } {}
			Item(const nullptr_t&) : Value{ nullptr } {}
			Item(const std::string& s) {
				std::wstring String;
				for (auto c : s) String.push_back(c);
				Value = String;
			}
			Item(const char* str) {
				std::wstring String;
				String.reserve(strlen(str));
				int i = 0;
				while (str[i])
					String.push_back(str[i++]);
				Value = String;
			}
			Item(const std::vector<unsigned char>& Binary) : Value{ Binary } {}

			Item(const jsArray& its);
			template<typename T>
			inline Item(const std::unordered_map<std::wstring, T>& Dictionary) {
				std::unordered_map<std::wstring, Item> Object;
				for (auto& [n, v] : Dictionary) Object[n] = (JSON::Item)v;
				Value = Object;
			}

			template<typename T>
			inline Item(const std::unordered_map<std::string, T>& Dictionary) {
				std::unordered_map<std::wstring, Item> Object;
				for (auto& [n, v] : Dictionary) Object[PVX::Encode::ToString(n)] = (JSON::Item)v;
				Value = Object;
			}

			template<typename T>
			inline Item(const std::map<std::wstring, T>& Dictionary) {
				std::unordered_map<std::wstring, Item> Object;
				for (auto& [n, v] : Dictionary) Object[n] = (JSON::Item)v;
				Value = Object;
			}

			template<typename T>
			inline Item(const std::map<std::string, T>& Dictionary) {
				std::unordered_map<std::wstring, Item> Object;
				for (auto& [n, v] : Dictionary) Object[PVX::Encode::ToString(n)] = (JSON::Item)v;
				Value = Object;
			}

			template<typename T>
			inline Item(const std::vector<T>& v) {
				std::vector<Item> Array;
				for (auto& i : v) Array.push_back((JSON::Item)i);
				Value = Array;
			}

			template<typename T>
			inline Item(const std::set<T>& v) {
				std::vector<Item> Array;
				for (auto& i : v) Array.push_back((JSON::Item)i);
				Value = Array;
			}

			inline Item(const std::initializer_list<std::tuple<std::wstring, Item>>& dict) {
				std::unordered_map<std::wstring, Item> val;
				for (auto& [n, v] : dict) val[n] = v;
				Value = val;
			}
			inline Item(const std::initializer_list<std::tuple<std::string, Item>>& dict) {
				std::unordered_map<std::wstring, Item> val;
				for (auto& [n2, v] : dict) {
					std::wstring n;
					n.reserve(n2.size());
					for (auto& c : n2) n.push_back(c);
					val[n] = v;
				}
				Value = val;
			}

			const Item& operator||(const Item& item) const;
			const Item& operator&&(const Item& item) const;

			Item& operator=(const enum jsElementType);
			Item& operator=(const nullptr_t&);
			Item& operator=(const int);
			Item& operator=(const long long);
			Item& operator=(const float);
			Item& operator=(const double);
			Item& operator=(const bool);
			Item& operator=(const std::wstring&);
			Item& operator=(const std::string&);
			Item& operator=(const wchar_t* str);
			Item& operator=(const char* str);
			Item& operator=(const std::vector<unsigned char>&);
			Item& operator=(const Item& obj) {
				Value = obj.Value;
				return *this;
			}

			void AddProperty(std::wstring&& Name, Item&& Val) {
				 Value.Object().insert({ std::forward<std::wstring>(Name), std::forward<Item>(Val) });
			}

			Item& operator[](const std::wstring&);
			Item& operator[](const std::string&);
			Item& operator[](int);
			const Item& operator[](const std::wstring&) const;
			const Item& operator[](const std::string&) const;
			const Item& operator[](int) const;
			Item Get(const std::wstring&, const Item& Default = jsElementType::Undefined) const;
			const Item* Has(const std::wstring&) const;

			Item& operator<<(const std::wstring&);

			jsElementType Type() const { return Value.GetType(); }
			BSON_Type BsonType() const { return Value.BsonType; }

			void push(const Item&);
			Item pop();
			int length() const;

			bool IsNull() const;
			bool IsUndefined() const;
			bool IsNullOrUndefined() const;
			bool IsEmpty() const;
			inline bool IsInteger() const { return Value.GetType() == jsElementType::Integer; }
			inline bool IsDouble() const { return Value.GetType() == jsElementType::Float; }

			std::vector<std::wstring> Keys() const;
			std::vector<PVX::JSON::Item> Values() const;

			double NumberSafeDouble() const;
			long long NumberSafeInteger() const;
			std::wstring GetString() const;

			long long& Integer() { return Value.Integer(); };
			long long Integer() const { return Value.Integer(); };
			double& Double() { return Value.Double(); };
			double Double()const { return Value.Double(); };
			bool& Boolean() { return Value.Boolean(); };
			bool Boolean() const { return Value.Boolean(); };
			std::wstring& String() { return Value.String(); }
			std::wstring String() const { return Value.String(); }

			operator std::wstring() const { return Value.String(); }
			operator const std::string() const { return PVX::Encode::ToString(Value.String()); }

			std::vector<unsigned char> Data();
			void Data(const std::vector<unsigned char>& d);

			template<typename T>
			auto map_T(T clb) const {
				std::vector<decltype(clb(Item()))> ret;
				ret.reserve(length());
				for (auto& x : Value.Value.Array->Array) ret.push_back(clb(x));
				return ret;
			}
			template<typename T>
			std::vector<T> map2_T(std::function<T(size_t,const Item&)> clb) const {
				std::vector<T> ret;
				ret.reserve(length());
				size_t i = 0;
				for (auto& x : Value.Value.Array->Array) ret.push_back(clb(i++, x));
				return ret;
			}

			Item map(std::function<Item(const Item&)> Convert);
			Item map2(std::function<Item(const Item&, int Index)> Convert);
			void each(std::function<void(Item&)> Func);
			void each(std::function<void(const Item&)> Func) const;
			void each2(std::function<void(Item&, int Index)> Func);
			void each2(std::function<void(const Item&, int Index)> Func) const;
			void eachInObject(std::function<void(const std::wstring& Name, Item&)> Func);
			void eachInObject(std::function<void(const std::wstring& Name, const Item&)> Func) const;
			Item GroupBy(std::function<std::wstring(const Item&)> Func);
			Item filter(std::function<int(const Item&)> Test);
			Item find(std::function<int(const Item&)> Test, size_t Start = 0);
			int findIndex(std::function<int(const Item&)> Test, size_t Start = 0);
			Item sort(std::function<int(Item&, Item&)> Compare);
			Item Copy();
			Item DeepCopy();
			Item DeepReducedCopy();

			Item& Merge(const Item& With);
			int SaveBinary(const wchar_t* Filename);
			int SaveBinary(const char* Filename);
			static Item ReadBinary(const char* Filename);
			static Item ReadBinary(const wchar_t* Filename);


			Variant Value;

			inline std::vector<Item>& getArray() {
				return Value.Array();
			}
			inline const std::vector<Item>& getArray() const {
				return Value.Array();
			}
			inline std::unordered_map<std::wstring, Item>& getObject() {
				return Value.Object();
			}
			inline const std::unordered_map<std::wstring, Item>& getObject() const {
				return Value.Object();
			}
		private:
			void WriteBin(void*);
			static Item ReadBin(void*);
		};


		class jsArray {
		protected:
			const std::initializer_list<Item>& itms;
			friend class Item;
		public:
			jsArray(const std::initializer_list<Item>& itm) : itms{ itm } {}
		};

		std::wstring stringify(const Item& Object, bool Format = false);
		Item parse(const unsigned char*, int size);
		Item parse(const std::vector<unsigned char>&);
		Item parse(const std::wstring_view& Json);
		Item parsePlus(const std::wstring_view& Json);





		JSON::Item fromBSON(const std::vector<unsigned char>& Data);
		JSON::Item fromBSON(const std::vector<unsigned char>& Data, size_t& Cursor);
		JSON::Item fromBSON(const std::wstring& Data);
		std::vector<unsigned char> ToBSON(const JSON::Item& obj);
		void ToBSON(const JSON::Item& obj, std::vector<unsigned char>& Data);
		JSON::Item ObjectId(const std::string_view& hexId);
		JSON::Item ObjectId(const std::wstring_view& hexId);
	}
	inline const JSON::Item operator ""_json(const wchar_t* Text, size_t sz) {
		return JSON::parse(Text);
	};
	namespace BSON {
		class BsonVariant;

		class Item {
		public:
			Item(const std::initializer_list<std::pair<std::wstring, BsonVariant>>& list);
			Item(const std::initializer_list<std::pair<std::string, BsonVariant>>& list);
			Item(const std::initializer_list<BsonVariant>& list);

			const std::vector<unsigned char> GetData() const { return Data; }
		protected:
			char Type;
			std::vector<unsigned char> Data;
			template<typename T>
			void write(T v) { auto cur = Data.size(); Data.resize(cur + sizeof(T)); (*(T*)&Data[cur]) = v; }
			void write(const std::string& str) { auto SizeIndex = AppendBytes(4); write2(str); (*(int*)&Data[SizeIndex]) = Data.size() - SizeIndex - 4; }
			void write(const std::wstring& str) { auto SizeIndex = AppendBytes(4); write2(str); (*(int*)&Data[SizeIndex]) = Data.size() - SizeIndex - 4; }
			void write(const Item& item) { auto cur = Data.size(); Data.resize(cur + item.Data.size()); memcpy(&Data[cur], item.Data.data(), item.Data.size()); }
			void write2(const std::string& str) { memcpy(&Data[AppendBytes(str.size() + 1)], str.data(), str.size()); }
			void write2(const std::wstring& str) { PVX::Encode::UTF(&Data[AppendBytes((int)PVX::Encode::UTF_Length(str) + 1)], str.c_str()); }
			size_t AppendBytes(int Count) { size_t cur = Data.size(); Data.resize(cur + Count); return cur; }
		};
		class BsonVariant {
			std::variant<
				std::string,
				std::wstring,
				bool,
				int32_t,
				int64_t,
				float,
				Item
			> Value;
			friend class Item;
		public:
			BsonVariant(const char* v) :Value{ std::string(v) } {}
			BsonVariant(const wchar_t* v) :Value{ std::wstring(v) } {}
			template<typename T>
			BsonVariant(const T& v) : Value{ v } {}
			template<int T>
			auto& get() { return std::get<T>(Value); }
			template<int T>
			const auto& get() const { return std::get<T>(Value); }
			int Index() const { return Value.index(); }
		};
	}
}
#endif