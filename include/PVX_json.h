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

		template<typename T>
		class R {
			std::shared_ptr<T> Val;
		public:
			R() = default;
			inline R(const T& v) : Val{ std::make_shared<T>(v) } {}
			T& operator=(const T& v) {
				Val = std::make_shared<T>(v);
				return *Val.get();
			}
			inline operator T& () {
				return *Val.get();
			}
			inline operator const T& () const {
				return *Val.get();
			}
			inline const T& operator()() const {
				return *Val.get();
			}
			inline T& operator()() {
				return *Val.get();
			}
			inline T* get() {
				return Val.get();
			}
			inline const T* get() const {
				return Val.get();
			}
		};

		class Item {
		protected:
			BSON_Type BSONType;
			jsElementType JSONType;
			int BinType = 0;
			std::variant<
				std::nullptr_t, 
				bool, 
				int32_t,
				int64_t,
				double, 
				std::wstring, 
				std::vector<Item>, 
				std::unordered_map<std::wstring, R<Item>>, 
				std::vector<uint8_t>
			> Value;
		public:
			Item() : Value{ nullptr }, JSONType { jsElementType::Undefined }, BSONType{ BSON_Type::Undefined } {}
			Item(const Item&) = default;
			Item(Item&&) = default;

			Item(const jsElementType& tp){
				JSONType = tp;
				switch (tp) {
					case jsElementType::Null: BSONType = BSON_Type::Null; Value = false; break;
					case jsElementType::Integer: BSONType = BSON_Type::Int64; Value = (int64_t)0; break;
					case jsElementType::Float: BSONType = BSON_Type::Double; Value = 0.0; break;
					case jsElementType::String: BSONType = BSON_Type::String; Value = L""; break;
					case jsElementType::Array: BSONType = BSON_Type::Array; Value = std::vector<Item>(); break;
					case jsElementType::Object: BSONType = BSON_Type::Object; Value = std::unordered_map<std::wstring, R<Item>>(); break;
					case jsElementType::Boolean: BSONType = BSON_Type::Boolean; Value = false; break;
					case jsElementType::Binary: BSONType = BSON_Type::Binary; Value = std::vector<uint8_t>(); break;
					default: BSONType = BSON_Type::Undefined; Value = false; break;
				}
			}
			Item(const bool& v) : Value{ v }, JSONType{ jsElementType::Boolean }, BSONType{ BSON_Type::Boolean } {}
			Item(const int& v) : Value{ v }, JSONType{ jsElementType::Integer }, BSONType{ BSON_Type::Int32 } {}
			Item(const int64_t& v) : Value{ v }, JSONType{ jsElementType::Integer }, BSONType{ BSON_Type::Int64 }{}
			Item(const uint64_t& v) : Value{ int64_t(v) }, JSONType{ jsElementType::Integer }, BSONType{ BSON_Type::Int64 } {}
			Item(const float& v) : Value{ double(v) }, JSONType{ jsElementType::Float }, BSONType{ BSON_Type::Double } {}
			Item(const double& v) : Value{ v }, JSONType{ jsElementType::Float }, BSONType{ BSON_Type::Double } {}
			Item(const std::wstring& s) : Value{ s }, JSONType{ jsElementType::String }, BSONType{ BSON_Type::String } {}
			Item(const wchar_t* s) : Value{ std::wstring(s) }, JSONType{ jsElementType::String }, BSONType{ BSON_Type::String } {}
			Item(const std::nullptr_t&) : Value{ nullptr }, JSONType{ jsElementType::Null }, BSONType{ BSON_Type::Null } {}
			Item(const std::string& s) : Value{ PVX::Encode::ToString(s) }, JSONType { jsElementType::String }, BSONType{ BSON_Type::String } {}
			Item(const char* str) : Value{ PVX::Encode::ToString(str) }, JSONType{ jsElementType::String }, BSONType{ BSON_Type::String } {}
			Item(const std::vector<unsigned char>& Binary) : Value{ Binary }, JSONType{ jsElementType::Binary }, BSONType{ BSON_Type::Binary } {}

			Item(const jsArray& its);

			template<typename T>
			inline Item(const std::unordered_map<std::wstring, T>& Dictionary) :
				JSONType{ JSON::jsElementType::Object }, 
				BSONType{ BSON_Type::Object },
				Value{ std::unordered_map<std::wstring, R<Item>>() }
			{
				auto& tmp = std::get<std::unordered_map<std::wstring, R<Item>>>(Value);
				for (auto& [n, v] : Dictionary)
					tmp.emplace(n, JSON::Item(v));
			}

			template<typename T>
			inline Item(const std::unordered_map<std::string, T>& Dictionary) :
				JSONType{ JSON::jsElementType::Object }, 
				BSONType{ BSON_Type::Object },
				Value{ std::unordered_map<std::wstring, R<Item>>() }
			{
				auto& tmp = std::get<std::unordered_map<std::wstring, R<Item>>>(Value);
				for (auto& [n, v] : Dictionary)
					tmp.emplace(PVX::Encode::ToString(n), JSON::Item(v));
			}

			template<typename T>
			inline Item(const std::map<std::wstring, T>& Dictionary) :
				JSONType{ JSON::jsElementType::Object }, 
				BSONType{ BSON_Type::Object },
				Value{ std::unordered_map<std::wstring, R<Item>>() }
			{
				auto& tmp = std::get<std::unordered_map<std::wstring, R<Item>>>(Value);
				for (auto& [n, v] : Dictionary)
					tmp.emplace(n, JSON::Item(v));
			}

			template<typename T>
			inline Item(const std::map<std::string, T>& Dictionary) :
				JSONType{ JSON::jsElementType::Object }, 
				BSONType{ BSON_Type::Object },
				Value{ std::unordered_map<std::wstring, R<Item>>() }
			{
				auto& tmp = std::get<std::unordered_map<std::wstring, R<Item>>>(Value);
				for (auto& [n, v] : Dictionary)
					tmp.emplace(PVX::Encode::ToString(n), JSON::Item(v));
			}

			template<typename T>
			inline Item(const std::vector<T>& v) :
				JSONType{ JSON::jsElementType::Array }, 
				BSONType{ BSON_Type::Array },
				Value{ [](const std::vector<T>& v) {
					std::vector<Item> tArray; 
					for (auto& i : v) 
						tArray.push_back(JSON::Item(i));
					return tArray; 
				}(v) }
			{}

			template<typename T>
			inline Item(const std::set<T>& v) :
				JSONType{ JSON::jsElementType::Array }, 
				BSONType{ BSON_Type::Array },
				Value{ [](const std::set<T>& v) {
					std::vector<Item> tArray;
					tArray.reserve(v.size());
					for (auto& i : v) 
						tArray.push_back((JSON::Item)i); 
					return tArray; 
				}(v) }
			{}

			inline Item(const std::initializer_list<std::tuple<std::wstring, Item>>& dict) :
				JSONType{ JSON::jsElementType::Object }, 
				BSONType{ BSON_Type::Object },
				Value{ [](const std::initializer_list<std::tuple<std::wstring, Item>>& dict) {
					std::unordered_map<std::wstring, R<Item>> val;
					for (auto& [n, v] : dict)
						val[n] = v;
					return val;
				}(dict)}
			{}
			inline Item(const std::initializer_list<std::tuple<std::string, Item>>& dict) :
				JSONType{ JSON::jsElementType::Object }, 
				BSONType{ BSON_Type::Object }, 
				Value{ [](const std::initializer_list<std::tuple<std::string, Item>>& dict) {
					std::unordered_map<std::wstring, R<Item>> val;
					for (auto& [n, v] : dict) 
						val[PVX::Encode::ToString(n)] = v;
					return val;
				}(dict)} 
			{}

			const Item operator||(const Item& item) const;
			const Item operator&&(const Item& item) const;

			Item& operator=(const jsElementType);
			Item& operator=(const std::nullptr_t);
			Item& operator=(const int);
			Item& operator=(const int64_t);
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
				BSONType = obj.BSONType;
				JSONType = obj.JSONType;
				return *this;
			}

			void AddProperty(std::wstring&& Name, Item&& Val) {
				std::get<std::unordered_map<std::wstring, R<Item>>>(Value).
					emplace(std::forward<std::wstring>(Name), std::forward<Item>(Val));
			}

			Item& operator[](const std::wstring&);
			Item& operator[](const std::string&);
			Item& operator[](int);
			const Item& operator[](const std::wstring&) const;
			const Item& operator[](const std::string&) const;
			const Item& operator[](int) const;
			Item Get(const std::wstring&, const Item& Default = jsElementType::Undefined) const;
			const Item* Has(const std::wstring&) const;
			Item* Has(const std::wstring&);

			inline void Delete(const std::wstring& Name) {
				if (Type() == PVX::JSON::jsElementType::Object) {
					getObject().erase(Name);
				}
			}

			template<typename T>
			inline std::vector<T> ToVector(std::function<T(const Item&)> clb) const {
				std::vector<T> ret;
				if (JSONType == JSON::jsElementType::Array) {
					ret.reserve(length());
					for (auto& i : getArray()) {
						ret.push_back(clb(i));
					}
				}
				return ret;
			}

			inline bool Boolean(const std::wstring& Name) const {
				if (JSONType == PVX::JSON::jsElementType::Object) {
					if (auto h = Has(Name); h) return h->Boolean();
				}
				return false;
			}
			inline const std::wstring String(const std::wstring& Name) const {
				if (JSONType == PVX::JSON::jsElementType::Object) {
					if (auto h = Has(Name); h) return h->String();
				}
				return L"";
			}
			inline double Double(const std::wstring& Name) const {
				if (JSONType == PVX::JSON::jsElementType::Object) {
					if (auto h = Has(Name); h) return h->NumberSafeDouble();
				}
				return 0;
			}
			inline int64_t Integer(const std::wstring& Name) const {
				if (JSONType == PVX::JSON::jsElementType::Object) {
					if (auto h = Has(Name); h) return h->NumberSafeInteger();
				}
				return 0;
			}

			bool If(const std::wstring& Name, std::function<void(Item&)> Then);
			bool If(const std::wstring& Name, std::function<void(const Item&)> Then) const;

			Item& operator<<(const std::wstring&);

			jsElementType Type() const { return JSONType; }
			BSON_Type BsonType() const { return BSONType; }

			jsElementType& Type() { return JSONType; }
			BSON_Type& BsonType() { return BSONType; }

			int& BinaryType() { return BinType; }
			int BinaryType() const { return BinType; }

			void push(const Item&);
			Item pop();
			int length() const;

			bool IsNull() const;
			bool IsUndefined() const;
			bool IsNullOrUndefined() const;
			bool IsEmpty() const;
			inline bool IsInteger() const { return JSONType == jsElementType::Integer; }
			inline bool IsDouble() const { return JSONType == jsElementType::Float; }
			inline bool IsObject() const { return JSONType == jsElementType::Object; }
			inline bool IsArray() const { return JSONType == jsElementType::Array; }

			std::vector<std::wstring> Keys() const;
			std::vector<Item> Values() const;

			double NumberSafeDouble() const;
			int64_t NumberSafeInteger() const;
			std::wstring GetString() const;

			inline int32_t& Int32() { return std::get<int32_t>(Value); };
			inline int64_t& Int64() { return std::get<int64_t>(Value); };
			inline int64_t Integer() const { 
				if(BSONType==BSON_Type::Int64)
					return std::get<int64_t>(Value); 
				else
					return std::get<int32_t>(Value);
			};
			inline double& Double() { return std::get<double>(Value); };
			inline double Double() const { return std::get<double>(Value); };
			inline bool& Boolean() { return std::get<bool>(Value); };
			inline bool Boolean() const { return std::get<bool>(Value); };
			inline std::wstring& String() { return std::get<std::wstring>(Value); }
			inline std::wstring String() const { return std::get<std::wstring>(Value); }
			inline std::unordered_map<std::wstring, R<Item>>& Object() { return std::get<std::unordered_map<std::wstring, R<Item>>>(Value); }
			inline const std::unordered_map<std::wstring, R<Item>>& Object() const { return std::get<std::unordered_map<std::wstring, R<Item>>>(Value); }

			inline std::vector<uint8_t>& Binary() { return std::get<std::vector<uint8_t>>(Value); }
			inline const std::vector<uint8_t>& Binary() const { return std::get<std::vector<uint8_t>>(Value); }

			inline std::vector<Item>& Array() { return std::get<std::vector<Item>>(Value); }
			inline const std::vector<Item>& Array() const { return std::get<std::vector<Item>>(Value); }

			operator std::wstring() const { return std::get<std::wstring>(Value); }
			operator const std::string() const { return PVX::Encode::ToString(std::get<std::wstring>(Value)); }

			//int64_t& Integer() { return Value.Integer(); };
			//int64_t Integer() const { return Value.Integer(); };
			//double& Double() { return Value.Double(); };
			//double Double()const { return Value.Double(); };
			//bool& Boolean() { return Value.Boolean(); };
			//bool Boolean() const { return Value.Boolean(); };
			//std::wstring& String() { return Value.String(); }
			//std::wstring String() const { return Value.String(); }

			//operator std::wstring() const { return Value.String(); }
			//operator const std::string() const { return PVX::Encode::ToString(Value.String()); }

			std::vector<unsigned char> Data();
			void Data(const std::vector<unsigned char>& d);

			template<typename T>
			auto map_T(T clb) const {
				std::vector<decltype(clb(Item()))> ret;
				ret.reserve(length());
				for (auto& x : std::get<std::vector<Item>>(Value)) ret.push_back(clb(x));
				return ret;
			}
			template<typename T>
			std::vector<T> map2_T(std::function<T(size_t,const Item&)> clb) const {
				std::vector<T> ret;
				ret.reserve(length());
				size_t i = 0;
				for (auto& x : std::get<std::vector<Item>>(Value)) ret.push_back(clb(i++, x));
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
			int64_t findIndex(std::function<int(const Item&)> Test, size_t Start = 0);
			Item sort(std::function<int(Item&, Item&)> Compare);
			Item Copy();
			Item DeepCopy();
			Item DeepReducedCopy();

			Item& Merge(const Item& With);
			int SaveBinary(const wchar_t* Filename);
			int SaveBinary(const char* Filename);
			static Item ReadBinary(const char* Filename);
			static Item ReadBinary(const wchar_t* Filename);

			//Variant Value;

			inline std::vector<Item>& getArray() {
				return std::get<std::vector<Item>>(Value);
				//return Value.Array();
			}
			inline const std::vector<Item>& getArray() const {
				return std::get<std::vector<Item>>(Value);
				//return Value.Array();
			}
			inline std::unordered_map<std::wstring, R<Item>>& getObject() {
				return std::get<std::unordered_map<std::wstring, R<Item>>>(Value);
				//return Value.Object();
			}
			inline const std::unordered_map<std::wstring, R<Item>>& getObject() const {
				return std::get<std::unordered_map<std::wstring, R<Item>>>(Value);
				//return Value.Object();
			}
		private:
			void WriteBin(std::ofstream& fout);
			static Item ReadBin(std::ifstream& fin);
		};


		class jsArray {
		protected:
			const std::initializer_list<Item>& itms;
			friend class Item;
		public:
			jsArray(const std::initializer_list<Item>& itm) : itms{ itm } {}
		};

		std::wstring stringify(const Item& Object, bool Format = false);
		Item parse(const unsigned char*, size_t size);
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
		JSON::Item Binary(const std::vector<unsigned char>& Data, int Type = 0);

		inline const JSON::Item EmptyObject() { return PVX::JSON::jsElementType::Object; }
		inline const JSON::Item EmptyArray() { return PVX::JSON::jsElementType::Array; }
	}

	namespace BSON {
		class Item;

		class BsonVariant {
			std::variant<
				std::string,
				std::wstring,
				bool,
				int32_t,
				int64_t,
				float,
				std::unique_ptr<Item>
			> Value;
			friend class Item;
		public:
			BsonVariant(const char* v) :Value{ std::string(v) } {}
			BsonVariant(const wchar_t* v) :Value{ std::wstring(v) } {}
			BsonVariant(const Item& v) :Value{ std::make_unique<Item>(v) } {}
			template<typename T>
			BsonVariant(const T& v) : Value{ v } {}
			template<int T>
			auto& get() { return std::get<T>(Value); }
			template<int T>
			const auto& get() const { return std::get<T>(Value); }
			int Index() const { return int(Value.index()); }
		};

		class Item {
		public:
			Item(const std::initializer_list<std::pair<std::wstring, const BsonVariant&>>& list);
			Item(const std::initializer_list<std::pair<std::string, const BsonVariant&>>& list);
			Item(const std::initializer_list<BsonVariant>& list);

			const std::vector<unsigned char> GetData() const { return Data; }
		protected:
			char Type;
			std::vector<unsigned char> Data;
			template<typename T>
			void write(T v) { auto cur = Data.size(); Data.resize(cur + sizeof(T)); (*(T*)&Data[cur]) = v; }
			void write(const std::string& str) { auto SizeIndex = AppendBytes(4); write2(str); (*(int*)&Data[SizeIndex]) = int(Data.size() - SizeIndex) - 4; }
			void write(const std::wstring& str) { auto SizeIndex = AppendBytes(4); write2(str); (*(int*)&Data[SizeIndex]) = int(Data.size() - SizeIndex) - 4; }
			void write(const Item& item) { auto cur = Data.size(); Data.resize(cur + item.Data.size()); memcpy(&Data[cur], item.Data.data(), item.Data.size()); }
			void write2(const std::string& str) { memcpy(&Data[AppendBytes(int(str.size()) + 1)], str.data(), str.size()); }
			void write2(const std::wstring& str) { PVX::Encode::UTF(&Data[AppendBytes((int)PVX::Encode::UTF_Length(str) + 1)], str.c_str()); }
			size_t AppendBytes(int Count) { size_t cur = Data.size(); Data.resize(cur + Count); return cur; }
		};
	}
}

inline const PVX::JSON::Item operator ""_json(const wchar_t* Text, size_t sz) { return PVX::JSON::parse(Text); };
#endif