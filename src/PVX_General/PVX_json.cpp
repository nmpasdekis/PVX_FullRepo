#include<PVX_json.h>
#include<PVX_Encode.h>
#include<sstream>
#include<fstream>
//#include <stdio.h>
#include <PVX_String.h>
#include <PVX_Encode.h>
#include <PVX_File.h>
#include <string_view>
#include <PVX_Regex.h>
#include <PVX.inl>

static std::wstring JsonString(const std::wstring& s) {
	std::wstringstream ret;
	ret << '"';
	for (auto c : s) {
		switch (c) {
			case '"': ret << "\\\""; break;
			case '\n': ret << "\\n"; break;
			case '\t': ret << "\\t"; break;
			case '\r': ret << "\\r"; break;
			case '\\': ret << "\\\\"; break;
			case 0:
				goto forceEnd;
			default: ret << c;
		}
	}
forceEnd:
	ret << '"';
	return ret.str();
}
enum class Symbols : wchar_t {
	Terminator = 0,
	Quote,
	Integer,
	Float,
	True,
	False,
	Null,
	OpenCurly,
	OpenSquare,
	CloseSquare,
	CloseCurly,
	Comma,
	Colon,
	Function,
};

namespace PVX {
	namespace JSON {
		static void WriteNumber(std::ofstream& fout, size_t n) {
			fout.write((char*)&n, sizeof(int32_t));
		}
		static void WriteString(std::ofstream& fout, const std::wstring& s) {
			WriteNumber(fout, s.size());
			fout.write((char*)s.data(), sizeof(wchar_t) * s.size());
		}
		void Item::WriteBin(std::ofstream& fout) {
			WriteNumber(fout, (size_t)JSONType);
			switch (JSONType) {
				case JSON::jsElementType::Integer: 
					if(BSONType==BSON_Type::Int32) fout.write((char*)&Int32(), sizeof(int32_t)); 
					else fout.write((char*)&Int64(), sizeof(int64_t));
					break;
				case JSON::jsElementType::Float: fout.write((char*)&Double(), sizeof(double)); break;
				case JSON::jsElementType::String: WriteString(fout, String()); break;
				case JSON::jsElementType::Array:
				{
					auto& tmpArray = Array();
					WriteNumber(fout, tmpArray.size());
					for (auto& i : tmpArray)
						i.WriteBin(fout);
					break;
				}
				case JSON::jsElementType::Object:
				{
					auto& tmpObject = Object();
					WriteNumber(fout, tmpObject.size());
					for (auto& [n, v] : tmpObject) {
						if (v().Type() != JSON::jsElementType::Undefined && v().Type()!=JSON::jsElementType::Null)
							WriteString(fout, n);
						v().WriteBin(fout);
					}
					break;
				}
				default: return;
			};
		}
#ifndef __linux
		int Item::SaveBinary(const wchar_t* Filename) {
			std::ofstream fout(Filename, std::ofstream::binary);
			if (fout.is_open()) WriteBin(fout);
			return 0;
		}
#endif
		int Item::SaveBinary(const char* Filename) {
			std::ofstream fout(Filename, std::ofstream::binary);
			if (fout.is_open()) WriteBin(fout);
			return 0;
		}


		static double ReadDouble(std::ifstream& fin) {
			double ret;
			fin.read((char*)&ret, sizeof(double));
			return ret;
		}
		static size_t ReadInt(std::ifstream& fin) {
			size_t ret = 0;
			fin.read((char*)&ret, sizeof(size_t));
			return ret;
		}
		static int64_t ReadLong(std::ifstream& fin) {
			int64_t ret = 0;
			fin.read((char*)&ret, sizeof(int64_t));
			return ret;
		}
		static std::wstring ReadString(std::ifstream& fin) {
			size_t tmp = 0;
			fin.read((char*)&tmp, 4);
			std::wstring ret;
			ret.resize(tmp);
			fin.read((char*)&ret[0], sizeof(wchar_t) * tmp);
			return ret;
		}
		Item Item::ReadBin(std::ifstream& fin) {
			JSON::jsElementType type = (JSON::jsElementType)ReadInt(fin);
			size_t count;
			switch (type) {
				case JSON::jsElementType::Integer: return ReadLong(fin);
				case JSON::jsElementType::Float: return ReadDouble(fin);
				case JSON::jsElementType::String: return ReadString(fin);
				case JSON::jsElementType::Array:
				{
					count = ReadInt(fin);
					Item ret = JSON::jsElementType::Array;
					auto& Array = ret.Array();
					for (auto i = 0; i < count; i++) Array.push_back(ReadBin(fin));
					return ret;
				};
				case JSON::jsElementType::Object:
				{
					count = ReadInt(fin);
					Item ret = JSON::jsElementType::Object;
					auto& Object = ret.Object();
					for (auto i = 0; i < count; i++) Object[ReadString(fin)] = ReadBin(fin);
					return ret;
				};
				default: break;
			};
			return type;
		}
		Item Item::ReadBinary(const char* Filename) {
			std::ifstream fin(Filename, std::ifstream::binary);
			if (fin.is_open()) {
				return ReadBin(fin);
			}
			return jsElementType::Undefined;
		}
#ifndef __linux
		Item Item::ReadBinary(const wchar_t* Filename) {
			std::ifstream fin(Filename, std::ifstream::binary);
			if (fin.is_open()) return ReadBin(fin);
			return jsElementType::Undefined;
		}
#endif

		Item::Item(const jsArray& its) {
			JSONType = jsElementType::Array;
			BSONType = BSON_Type::Array;
			std::vector<Item> Array;
			for (auto& it : its.itms) Array.push_back(it);
			Value = Array;
		}

		//static std::wstring wstr(const std::string& str) {
		//	std::wstring ret; ret.resize(str.size());
		//	memcpy(&ret[0], &str[0], str.size());
		//	return ret;
		//}
		Item& Item::operator=(const jsElementType tp) {
			JSONType = tp;
			switch (tp) {
				case jsElementType::Undefined: BSONType = BSON_Type::Undefined; Value = false; break;
				case jsElementType::Null: BSONType = BSON_Type::Null; Value = false; break;
				case jsElementType::Integer: BSONType = BSON_Type::Int64; Value = (int64_t)0; break;
				case jsElementType::Float: BSONType = BSON_Type::Double; Value = 0.0; break;
				case jsElementType::String: BSONType = BSON_Type::String; Value = L""; break;
				case jsElementType::Array: BSONType = BSON_Type::Array; Value = std::vector<Item>(); break;
				case jsElementType::Object: BSONType = BSON_Type::Object; Value = std::unordered_map<std::wstring, R<Item>>(); break;
				case jsElementType::Boolean: BSONType = BSON_Type::Boolean; Value = false; break;
				case jsElementType::Binary: BSONType = BSON_Type::Binary; Value = std::vector<uint8_t>(); break;
			}
			return *this;
		}
		const Item Item::operator||(const Item& item) const {
			if (!IsEmpty()) return *this;
			return item;
		}
		const Item Item::operator&&(const Item& item) const {
			if (IsEmpty()) return false;
			return item;
		}
		Item& Item::operator=(const std::nullptr_t n) {
			Value = nullptr;
			JSONType = jsElementType::Null;
			BSONType = BSON_Type::Null;
			return *this;
		}
		Item& Item::operator=(const int v) {
			Value = v;
			JSONType = jsElementType::Integer;
			BSONType = BSON_Type::Int32;
			return *this;
		}
		Item& Item::operator=(const int64_t v) {
			Value = v;
			JSONType = jsElementType::Integer;
			BSONType = BSON_Type::Int64;
			return *this;
		}
		Item& Item::operator=(const float v) {
			Value = (double)v;
			JSONType = jsElementType::Float;
			BSONType = BSON_Type::Double;
			return *this;
		}
		Item& Item::operator=(const double v) {
			Value = v;
			JSONType = jsElementType::Float;
			BSONType = BSON_Type::Double;
			return *this;
		}
		Item& Item::operator=(const bool v) {
			Value = v;
			JSONType = jsElementType::Boolean;
			BSONType = BSON_Type::Boolean;
			return *this;
		}
		Item& Item::operator=(const std::wstring& v) {
			Value = v;
			JSONType = jsElementType::String;
			BSONType = BSON_Type::String;
			return *this;
		}

		Item& Item::operator=(const std::string& s) {
			Value = PVX::Encode::ToString(s);
			JSONType = jsElementType::String;
			BSONType = BSON_Type::String;
			return *this;
		}
		Item& Item::operator=(const wchar_t* v) {
			Value = std::wstring(v);
			JSONType = jsElementType::String;
			BSONType = BSON_Type::String;
			return *this;
		}

		Item& Item::operator=(const char* s) {
			Value = PVX::Encode::ToString(s);
			JSONType = jsElementType::String;
			BSONType = BSON_Type::String;
			return *this;
		}

		Item& Item::operator=(const std::vector<unsigned char>& v) {
			Value = v;
			JSONType = jsElementType::Binary;
			BSONType = BSON_Type::Binary;
			return *this;
		}


		Item& Item::operator[](const std::wstring& Name) {
			return Object()[Name]();
		}
		const Item& Item::operator[](const std::wstring& Name) const {
			return Object().at(Name)();
		}

		Item& Item::operator[](const std::string& Name) {
			std::wstring n;
			for (auto c : Name) n.push_back(wchar_t(c));
			return Object()[n];
		}
		const Item& Item::operator[](const std::string& Name) const {
			std::wstring n;
			for (auto c : Name)n.push_back(c);
			return Object().at(n)();
		}

		Item& Item::operator[](int Index) {
			return Array()[Index];
		}
		const Item& Item::operator[](int Index) const {
			return Array()[Index];
		}

		Item Item::Get(const std::wstring& Name, const Item& Default) const {
			auto& object = Object();
			if (auto ret = object.find(Name); ret!=object.end())
				return ret->second;
			return Default;
		}

		const Item* Item::Has(const std::wstring& Name) const {
			if (IsObject()) {
				auto& object = Object();
				if (auto ret = object.find(Name); ret != object.end())
					return ret->second.get();
			}
			return nullptr;
		}
		Item* Item::Has(const std::wstring& Name) {
			if (IsObject()) {
				auto& object = Object();
				if (auto ret = object.find(Name); ret != object.end())
					return ret->second.get();
			}
			return nullptr;
		}

		bool Item::If(const std::wstring& Name, std::function<void(JSON::Item&)> Then) {
			if (IsObject()) {
				if (auto it = Has(Name); it) { Then(*it); return true; }
			}
			return false;
		}
		bool Item::If(const std::wstring& Name, std::function<void(const JSON::Item&)> Then) const {
			if (IsObject()) {
				if (auto it = Has(Name); it) { Then(*it); return true; }
			}
			return false;
		}

		Item& Item::operator<<(const std::wstring& s) {
			*this = parse(s.c_str());
			return *this;
		}

		void Item::push(const Item& it) {
			Array().emplace_back(it);
		}

		Item Item::pop() {
			auto& tArray = Array();
			auto ret = tArray.back();
			tArray.pop_back();
			return ret;
		}

		int Item::length() const {
			return (int)Array().size();
		}

		bool Item::IsNull() const {
			return Type() == jsElementType::Null;
		}
		bool Item::IsUndefined() const {
			return Type() == jsElementType::Undefined;
		}
		bool Item::IsNullOrUndefined() const {
			return Type() == jsElementType::Null || Type() == jsElementType::Undefined;
		}
		bool Item::IsEmpty()  const {
			auto type = Type();
			return
				type == jsElementType::Null ||
				type == jsElementType::Undefined ||
				((type == jsElementType::Integer || type == jsElementType::Boolean || type == jsElementType::Float) && !Integer()) ||
				(type == jsElementType::String && !String().size()) ||
				(type == jsElementType::Array && !Array().size()) ||
				(type == jsElementType::Object && !Object().size());
		}

		std::vector<std::wstring> Item::Keys() const {
			std::vector<std::wstring> ret;
			for (auto& kv : Object())
				ret.push_back(kv.first);
			return ret;
		}
		std::vector<PVX::JSON::Item> Item::Values() const {
			std::vector<PVX::JSON::Item> ret;
			for (auto& kv : Object())
				ret.push_back(kv.second);
			return ret;
		}

		double Item::NumberSafeDouble() const {
			switch (Type()) {
				case jsElementType::Float: return Double();
				case jsElementType::Integer: return (double)Integer();
				case jsElementType::String: return std::stod(String());
				default: return 0.0;
			}
		}

		int64_t Item::NumberSafeInteger() const {
			switch (Type()) {
				case jsElementType::Float: return (int64_t)Double();
				case jsElementType::Integer: return Integer();
				case jsElementType::String: return std::stoll(String());
				default: return 0ll;
			}
		}

		std::vector<unsigned char> Item::Data() {
			return PVX::Decode::Base64(String());
		}

		std::wstring Item::GetString() const {
			if (Type()== jsElementType::String)
				return String();
			return stringify(*this);
		}

		void Item::Data(const std::vector<unsigned char>& d) {
			Value = PVX::Encode::ToString(PVX::Encode::Base64(d));
		}

		Item Item::map(std::function<Item(const Item&)> Convert) {
			if (Type() == JSON::jsElementType::Array) {
				Item ret = JSON::jsElementType::Array;
				ret.Array().reserve(Array().size());
				for (auto& i : Array()) {
					ret.push(Convert(i));
				}
				return ret;
			}
			return jsElementType::Undefined;
		}

		Item Item::map2(std::function<Item(const Item&, int Index)> Convert) {
			if (Type() == JSON::jsElementType::Array) {
				Item ret = JSON::jsElementType::Array;
				ret.Array().reserve(Array().size());
				int Index = 0;
				for (auto& i : Array()) {
					ret.push(Convert(i, Index++));
				}
				return ret;
			}
			return jsElementType::Undefined;
		}

		void Item::each(std::function<void(Item&)> Func) {
			if (Type() == JSON::jsElementType::Array) for (auto& i : Array()) Func(i);
		}
		void Item::each(std::function<void(const Item&)> Func) const {
			if (Type() == JSON::jsElementType::Array) for (auto& i : Array()) Func(i);
		}
		void Item::each2(std::function<void(Item&, size_t Index)> Func) {
			size_t Index = 0;
			if (Type() == JSON::jsElementType::Array) for (auto& i : Array()) Func(i, Index++);
		}
		void Item::each2(std::function<void(const Item&, size_t Index)> Func) const {
			size_t Index = 0;
			if (Type() == JSON::jsElementType::Array) for (auto& i : Array()) Func(i, Index++);
		}
		void Item::eachInObject(std::function<void(const std::wstring& Name, Item&)> Func) {
			if (Type()!=jsElementType::Object)return;
			for (auto& [Name, Value] : Object()) Func(Name, Value);
		}
		void Item::eachInObject(std::function<void(const std::wstring& Name, const Item&)> Func) const {
			if (Type()!=jsElementType::Object)return;
			for (const auto& [Name, Value] : Object()) Func(Name, Value);
		}
		Item Item::GroupBy(std::function<std::wstring(const Item&)> Func) {
			if (Type()!=jsElementType::Array) return jsElementType::Undefined;
			Item ret = jsElementType::Object;
			each([&](const Item& it) {
				std::wstring Name = Func(it);
				if (!ret.Has(Name)) ret[Name] = jsElementType::Array;
				ret[Name].push(it);
			});
			return ret;
		}

		Item Item::filter(std::function<int(const Item&)> Test) {
			if (Type() == JSON::jsElementType::Array) {
				Item ret = JSON::jsElementType::Array;
				ret.Array().reserve(Array().size());
				for (auto& i : Array()) {
					if (Test(i))
						ret.push(i);
				}
				ret.Array().shrink_to_fit();
				return ret;
			}
			return jsElementType::Undefined;
		}
		Item Item::find(std::function<int(const Item&)> Test, size_t Start) {
			auto& tArray = Array();
			if (Type() == JSON::jsElementType::Array) for (auto i = Start; i < tArray.size(); i++)
				if (Test(tArray[i])) return tArray[i];
			return jsElementType::Undefined;
		}

		int64_t Item::findIndex(std::function<int(const Item&)> Test, size_t Start) {
			auto& tArray = Array();
			if (Type() == JSON::jsElementType::Array) for (int64_t i = Start; i < (int64_t)tArray.size(); i++)
				if (Test(tArray[i])) return i;
			return -1;
		}

		Item Item::sort(std::function<int(Item&, Item&)> Compare) {
			if (Type() == JSON::jsElementType::Array) {
				Item ret = Copy();
				std::sort(ret.Array().begin(), ret.Array().end(), Compare);
				return ret;
			}
			return jsElementType::Undefined;
		}

		Item Item::Copy() {
			Item ret = Type();
			ret.Value = Value;
			return ret;
		}

		Item Item::DeepCopy() {
			switch (Type()) {
				case PVX::JSON::jsElementType::Undefined:
				case PVX::JSON::jsElementType::Null:
				case PVX::JSON::jsElementType::Float:
				case PVX::JSON::jsElementType::Integer:
				case PVX::JSON::jsElementType::String:
				case PVX::JSON::jsElementType::Boolean: return (*this);
				case PVX::JSON::jsElementType::Array:
					return map([](auto x) { return x.DeepCopy(); });
					break;
				case PVX::JSON::jsElementType::Object: {
					Item ret = PVX::JSON::jsElementType::Object;
					for (auto& [k, v]: Object()) {
						ret[k] = v().DeepCopy();
					}
					return ret;
				}
				case PVX::JSON::jsElementType::Binary: {
					Item ret = PVX::JSON::jsElementType::Binary;
					auto& dst = ret.Binary();
					auto& src = Binary();
					dst.resize(src.size());
					memcpy(dst.data(), src.data(), src.size());
					return ret;
				}
			}
			return PVX::JSON::jsElementType::Undefined;
		}
		Item Item::DeepReducedCopy() {
			switch (Type()) {
				case PVX::JSON::jsElementType::Undefined:
				case PVX::JSON::jsElementType::Null:
					return jsElementType::Undefined;
				case PVX::JSON::jsElementType::Float:
				case PVX::JSON::jsElementType::Integer:
				case PVX::JSON::jsElementType::String:
				case PVX::JSON::jsElementType::Boolean:
					if (IsEmpty()) return jsElementType::Undefined;
					return (*this);
				case PVX::JSON::jsElementType::Array:
				{
					Item ret = jsElementType::Array;
					int count = 0;
					for (auto& x: Array()) {
						auto y = x.DeepReducedCopy();
						count += y.Type() == jsElementType::Undefined;
						ret.push(y);
					}
					if (count == length()) return jsElementType::Undefined;
					return ret;
				}
				case PVX::JSON::jsElementType::Object: {
					Item ret = jsElementType::Object;
					for (auto& [k, v]: Object()) {
						auto x = v().DeepReducedCopy();
						if (x.Type() != jsElementType::Undefined)
							ret[k] = x;
					}
					return ret;
				}
				case PVX::JSON::jsElementType::Binary: {
					Item ret = PVX::JSON::jsElementType::Binary;
					auto& dst = ret.Binary();
					auto& src = Binary();
					dst.resize(src.size());
					memcpy(dst.data(), src.data(), src.size());
					return ret;
				}
			}
			return PVX::JSON::jsElementType::Undefined;
		}

		Item& Item::Merge(const Item& With) {
			if (this->Type() == jsElementType::Undefined) {
				(*this) = With;
				return *this;
			}
			if (this->Type() == With.Type()) {
				if (this->Type() == JSON::jsElementType::Object) {
					for (auto& it : With.Object())
						(*this)[it.first].Merge(it.second);
				} else if (this->Type() == JSON::jsElementType::Array) {
					for (auto& it : With.Array())
						push(it);
				}
			}
			return *this;
		}

		std::wstring _lvl(int l) {
			return L"\n" + std::wstring(l, L'\t');
		}

		std::wstring stringify(const Item& obj, int level, bool Format) {
			std::wstring Lvl = Format ? _lvl(level) : L"";
			std::wstring Lvl1 = Format ? _lvl(level + 1) : L"";
			std::wstring colon = Format ? L": " : L":";

			std::wstringstream ret;
			switch (obj.Type()) {
				case jsElementType::Undefined:
					return L"undefined";
				case jsElementType::Null:
					return L"null";
				case jsElementType::Float:
					return std::to_wstring((long double)obj.Double());
				case jsElementType::Integer:
					return std::to_wstring(obj.Integer());
				case jsElementType::Boolean:
					return obj.Boolean() ? L"true" : L"false";
				case jsElementType::String:
					return JsonString(obj.String());
				case jsElementType::Binary:
					switch (obj.BsonType()) {
						case BSON_Type::ObjectId:
							return L"ObjectId(\"" + PVX::Encode::wToHex(obj.Binary()) + L"\")";
						case BSON_Type::Binary:
							return L"Binary(\"" + PVX::Encode::wToHex(obj.Binary()) + L"\", " + std::to_wstring(obj.BinaryType()) + L")";
						default: break;
					}
					return L"undefined";
				case jsElementType::Array:
					ret << "[";
					{
						auto& tArray = obj.Array();
						if (tArray.size()) {
							size_t i = 0;
							while (i< tArray.size() && tArray[i].Type()==jsElementType::Undefined)i++;
							ret << Lvl1 << stringify(tArray[i], level + 1, Format); i++;
							while (i < tArray.size() && tArray[i].Type()==jsElementType::Undefined)i++;
							for (; i < tArray.size(); i++) {
								ret << "," << Lvl1 << stringify(tArray[i], level + 1, Format);
								while (i < tArray.size() && tArray[i].Type()==jsElementType::Undefined)i++;
							}
						}
					}
					ret << Lvl << "]";
					return ret.str();
				case jsElementType::Object:
				{
					ret << "{";
					auto& object = obj.Object();
					auto iter = object.begin();

					while (iter!=object.end() && iter->second().Type() == jsElementType::Undefined) iter++;

					if (iter != object.end()) {
						ret << Lvl1 << JsonString(iter->first) << colon << stringify(iter->second, level + 1, Format);
						++iter;
					}

					for (; iter != object.end(); ++iter) {
						if (iter->second().Type() != jsElementType::Undefined)
							ret << "," << Lvl1 << JsonString(iter->first) << colon << stringify(iter->second, level + 1, Format);
					}

					ret << Lvl << "}";
					return ret.str();
				}
				default:
					return L"";
			}
		}

		std::wstring stringify(const Item& obj, bool Format) {
			return stringify(obj, 0, Format);
		}
		Item parse(const uint8_t* data, size_t size) {
			if (!size)return jsElementType::Undefined;
			auto utf = Decode::UTF(data, size);
			return parse(utf.c_str());
		}
		Item parse(const std::vector<uint8_t>& d) {
			return parse(d.data(), d.size());
		}

		struct jsonStack {
			int op;
			int Empty = 0;
			PVX::JSON::Item val;
			jsonStack** Child = nullptr;
			jsonStack* Parent = nullptr;

			jsonStack() {
				op = 321;
			}
			jsonStack(int op, int empty, PVX::JSON::Item&& val) : op{ op }, Empty{ empty }, val{ std::move(val) }{}
			jsonStack(int op, char empty) : op{ op }, Empty{ empty }{}
			jsonStack(int op) : op{ op } {}
			jsonStack(const jsonStack&) = delete;
			jsonStack(jsonStack&& v) noexcept : op{ v.op }, Empty{ v.Empty }, val{ std::move(v.val) }, Child{ v.Child }{ v.Child = nullptr; }
			~jsonStack() { Release(); }
			void Release() {
				if (Child) {
					delete Child[0];
					delete Child[1];
					delete Child;
				}
			}
		};

		void MakeObject(Item& obj, jsonStack* s) {
			jsonStack* cur = s;
			jsonStack* tmp = s;
			if (s->op == (char)Symbols::Colon) {
				obj.AddProperty(std::move(s->Child[1]->val.String()), std::move(s->Child[0]->val));
				delete s->Child[0];
				delete s->Child[1];
				delete s->Child;
				s->Child = nullptr;
				return; 
			}
			cur = cur->Child[1];
			cur->Parent = tmp;

			while (cur!=s) {
				while (cur->op == (char)Symbols::Comma) {
					tmp = cur;
					cur = cur->Child[1];
					cur->Parent = tmp;
				}
				if (cur->op != (char)Symbols::Colon) {
					obj = jsElementType::Undefined;
					return;
				}
				obj.AddProperty(std::move(cur->Child[1]->val.String()), std::move(cur->Child[0]->val));
				delete cur->Child[0];
				delete cur->Child[1];
				delete cur->Child;
				cur->Child = nullptr;

				if (cur == cur->Parent->Child[1]) {
					tmp = cur->Parent;
					delete tmp->Child[1];
					tmp->Child[1] = nullptr;
					cur = tmp->Child[0];
					cur->Parent = tmp;
					continue;
				}
				while (cur!=s && cur == cur->Parent->Child[0]) {
					cur = cur->Parent;
					delete cur->Child[0];
					cur->Child[0] = nullptr;
					delete cur->Child;
					cur->Child = nullptr;
				}
				tmp = cur->Parent;
				if (!tmp) return;
				cur = tmp->Child[0];
				cur->Parent = tmp;
			}
		}
		void MakeArray(Item& obj, jsonStack* s) {
			jsonStack* cur = s;
			jsonStack* tmp = s;
			if (s->op != (char)Symbols::Comma) { obj.push(s->val); return; }
			cur = cur->Child[1];
			cur->Parent = tmp;

			while (cur!=s) {
				while (cur->op == (char)Symbols::Comma) {
					tmp = cur;
					cur = cur->Child[1];
					cur->Parent = tmp;
				}
				obj.push(std::move(cur->val));
				if (cur == cur->Parent->Child[1]) {
					tmp = cur->Parent;
					delete tmp->Child[1];
					tmp->Child[1] = nullptr;
					cur = tmp->Child[0];
					cur->Parent = tmp;
					continue;
				}
				while (cur!=s && cur == cur->Parent->Child[0]) {
					cur = cur->Parent;
					delete cur->Child[0];
					cur->Child[0] = nullptr;
					delete cur->Child;
					cur->Child = nullptr;
				}
				tmp = cur->Parent;
				if (!tmp) return;
				cur = tmp->Child[0];
				cur->Parent = tmp;
			}
		}

		static std::wstring RemoveStrings2(const std::wstring& txt, std::vector<std::wstring>& Strings) {
			std::wstring ret;
			int Escape = 0;
			int64_t Start = -1;
			int Out = 1;
			for (int64_t i = 0; i < (int64_t)txt.size(); i++) {
				auto c = txt[i];
				if (c == '\"' && !Escape) {
					if (Out) {
						Start = i + 1;
						Out = 0;
					} else {
						Strings.push_back(PVX::Decode::Unescape(txt.substr(Start, i - Start)));
						Out = 1;
					}
				}

				if (Out) ret.push_back(c);
				Escape = c == '\\';
			}
			return ret;
		}

		static std::wregex EscapeReplacer(LR"regex(\\[\\\"tnr\'])regex", std::regex_constants::optimize);

		static std::wstring RemoveStrings(const std::wstring_view& text, std::vector<std::wstring>& Strings) {
			std::wstring Text = text.data();
			auto index = Text.find('"');
			while (index != std::wstring::npos) {
				auto end = Text.find('"', index + 1);
				while (end != std::wstring::npos && Text[end - 1] == '\\') end = Text.find('"', end + 1);
				if (end == std::wstring::npos)
					return L"";

				auto txt = Text.substr(index + 1, end - index - 1);

				txt = PVX::Replace(txt, EscapeReplacer, [](const std::wstring& x) -> std::wstring {
					auto c = x[1];
					switch (c) {
						case 't': return L"\t";
						case 'n': return L"\n";
						case 'r': return L"\r";
					}
					return x.c_str() + 1;
				});

				Strings.push_back(txt);
				Text = Text.replace(index, end - index + 1, L"\x01");
				index = Text.find('"', index + 1);
			}
			return Text;
		}

		int64_t FindNum(std::wstring& txt, int64_t& cur, int64_t start) {
			if (cur==start)
				for (; txt[start]&&(txt[start]!='-'&&(txt[start]<L'0' || txt[start]>L'9')); start++, cur++);
			else
				for (; txt[start]&&(txt[start]!='-'&&(txt[start]<L'0' || txt[start]>L'9')); start++) {
					txt[cur++] = txt[start];
				}
			if (txt[start]) return start;
			//txt[cur] = 0;
			txt.resize(cur);
			return -1;
		}

		void removeNumbers(std::wstring& txt, std::vector<double>& Doubles, std::vector<int64_t>& Integers) {
			int64_t cur = 0;
			int64_t start = FindNum(txt, cur, 0);
			while (start != -1) {
				auto end = start + 1;
				while (txt[end]>=L'0' && txt[end]<=L'9') end++;
				if (txt[end]&&txt[end]==L'.') {
					end++;
					while (txt[end]>=L'0' && txt[end]<=L'9') end++;
					Doubles.push_back(std::stod(&txt[start]));
					txt[cur] = (wchar_t)Symbols::Float;
				} else {
					Integers.push_back(std::stoll(&txt[start]));
					txt[cur] = (wchar_t)Symbols::Integer;
				}
				cur++;

				start = FindNum(txt, cur, end);
			}
		}
		void removeSpaces(std::wstring& txt) {
			size_t j = 0;
			for (auto i = 0; i<txt.size(); i++) {
				if (txt[i]!=' '&&txt[i]!='\t'&&txt[i]!='\r'&&txt[i]!='\n') {
					txt[j++] = txt[i];
				}
			}
			txt.resize(j);
		}

		void removeBoolsAndNulls(std::wstring& txt) {
			std::vector<bool> ret;
			size_t j = 0;
			for (auto i = 0; i<txt.size(); i++) {
				if (txt[i] == 't') {
					ret.push_back(true);
					i += 3;
					txt[j++] = (wchar_t)Symbols::True;
					continue;
				} else if (txt[i] == 'f') {
					ret.push_back(false);
					i += 4;
					txt[j++] = (wchar_t)Symbols::False;
					continue;
				} else if (txt[i] == 'n') {
					i += 3;
					txt[j++] = (wchar_t)Symbols::Null;
					continue;
				}
				txt[j++] = txt[i];
			}
			txt.resize(j);
		}

		void RemoveFunctions(std::wstring& Text, std::vector<std::wstring>& Functions, std::vector<std::vector<std::wstring>>& Args) {
			using namespace std::string_literals;
			Text = PVX::Replace(Text, LR"regex(([$a-zA-Z_][$a-zA-Z_0-9]*)\(([^\)]*)\))regex", [&Functions, &Args](const std::wsmatch& m) {
				Functions.emplace_back(m[1].str());
				Args.emplace_back(PVX::String::Split_Trimed(m[2].str(), L","s));
				return L"@";
			});
		}

		Item parse(const std::wstring_view& Json) {
			std::vector<std::wstring> Strings;
			std::vector<int64_t> Integers;
			std::vector<double> Doubles;

			auto tmp = RemoveStrings(Json, Strings);
			removeSpaces(tmp);
			removeBoolsAndNulls(tmp);
			removeNumbers(tmp, Doubles, Integers);

			std::vector<jsonStack> Output, Stack2;
			Output.reserve(tmp.size());
			Stack2.reserve(tmp.size());
			std::vector<wchar_t> Stack;
			Stack.reserve(tmp.size());
			int ItemCount = 0;
			int ints = 0, floats = 0, strings = 0;

			for (auto& t : tmp) {
				switch (t) {
					case '{': t = (wchar_t)Symbols::OpenCurly; continue;
					case '[': t = (wchar_t)Symbols::OpenSquare; continue;
					case ']': t = (wchar_t)Symbols::CloseSquare; continue;
					case '}': t = (wchar_t)Symbols::CloseCurly; continue;
					case ',': t = (wchar_t)Symbols::Comma; continue;
					case ':': t = (wchar_t)Symbols::Colon; continue;
					case '@': t = (wchar_t)Symbols::Function; continue;
				}
			}

			for (auto c : tmp) {
				char Empty = 0;
				switch (c) {
					case (wchar_t)Symbols::Quote: Output.emplace_back(0, 0, std::move(Strings[strings++])); ItemCount++; break;
					case (wchar_t)Symbols::Integer: Output.emplace_back(0, 0, std::move(Integers[ints++])); ItemCount++; break;
					case (wchar_t)Symbols::Float: Output.emplace_back(0, 0, std::move(Doubles[floats++])); ItemCount++; break;
					case (wchar_t)Symbols::True: Output.emplace_back(0, 0, true); ItemCount++; break;
					case (wchar_t)Symbols::False: Output.emplace_back(0, 0, false); ItemCount++; break;
					case (wchar_t)Symbols::Null: Output.emplace_back(0, 0, jsElementType::Null); ItemCount++;  break;

					case (wchar_t)Symbols::OpenCurly: Stack.push_back((wchar_t)Symbols::OpenCurly); ItemCount = 0; break;
					case (wchar_t)Symbols::OpenSquare: Stack.push_back((wchar_t)Symbols::OpenSquare); ItemCount = 0; break;
					case (wchar_t)Symbols::CloseSquare: {
						if (Stack.size() && Stack.back() == (wchar_t)Symbols::OpenSquare && !ItemCount) Empty = 1;
						while (Stack.size() && Stack.back() != (wchar_t)Symbols::OpenSquare) {
							Output.emplace_back(Stack.back(), 0);
							Stack.pop_back();
						}
						if (!Stack.size()||Stack.back()!=(wchar_t)Symbols::OpenSquare)
							return jsElementType::Undefined;
						Stack.pop_back();
						Output.emplace_back((char)Symbols::OpenSquare, Empty, jsElementType::Array);
						ItemCount++;
						break;
					}
					case (wchar_t)Symbols::CloseCurly: {
						if (Stack.size() && Stack.back() == (wchar_t)Symbols::OpenCurly && !ItemCount) Empty = 1;
						while (Stack.size() && Stack.back() != (wchar_t)Symbols::OpenCurly) {
							Output.emplace_back(Stack.back());
							Stack.pop_back();
						}
						if (!Stack.size() || Stack.back() != (wchar_t)Symbols::OpenCurly)
							return jsElementType::Undefined;
						Stack.pop_back();
						Output.emplace_back((wchar_t)Symbols::OpenCurly, Empty, jsElementType::Object);
						ItemCount++;
						break;
					}
					case (wchar_t)Symbols::Comma:
						while (Stack.size() && Stack.back() > (wchar_t)Symbols::Comma) {
							Output.emplace_back(Stack.back());
							Stack.pop_back();
						}
						Stack.push_back(c);
						break;
					case (wchar_t)Symbols::Colon:
						Stack.push_back(c);
						break;
				}
			}
			while (Stack.size()) {
				Output.emplace_back(Stack.back());
				//Output.push_back({ Stack.back() });
				Stack.pop_back();
			}

			for (auto& s : Output) {
				switch (s.op) {
					case 0:	Stack2.emplace_back(std::move(s)); break;
					case (wchar_t)Symbols::Colon:
					case (wchar_t)Symbols::Comma: {
						if (Stack2.size() >= 2) {
							//s.Child.emplace_back(std::move(Stack2.back()));
							//Stack2.pop_back();
							//s.Child.emplace_back(std::move(Stack2.back()));
							//Stack2.pop_back();

							s.Child = new jsonStack*[2]{
								new jsonStack(std::move(Stack2.back())),
								new jsonStack(std::move(Stack2[Stack2.size()-2]))
							};
							Stack2.resize(Stack2.size()-2);

							Stack2.emplace_back(std::move(s));
							//Stack2.back() = std::move(s);
							break;
						}
						return jsElementType::Undefined;
					}
					case (wchar_t)Symbols::OpenCurly: {
						if (s.Empty) {
							Stack2.emplace_back(std::move(s));
							break;
						} else if (Stack2.size()) {
							//MakeObject(s.val, std::move(Stack2.back()));
							MakeObject(s.val, &Stack2.back());

							Stack2.pop_back();
							Stack2.emplace_back(std::move(s));
							//Stack2.back() = std::move(s);
							break;
						}
						return jsElementType::Undefined;
					}
					case (wchar_t)Symbols::OpenSquare: {
						if (s.Empty) {
							Stack2.emplace_back(std::move(s));
							break;
						} else if (Stack2.size()) {
							//MakeArray(s.val, std::move(Stack2.back()));
							MakeArray(s.val, &Stack2.back());

							Stack2.pop_back();
							Stack2.emplace_back(std::move(s));
							//Stack2.back() = std::move(s);
							break;
						}
						return jsElementType::Undefined;
					}
				}
			}
			if (Stack2.size() == 1) return Stack2[0].val;
			return jsElementType::Undefined;
		}

		Item parsePlus(const std::wstring_view& Json) {
			std::vector<std::wstring> Functions;
			std::vector<std::vector<std::wstring>> FunctionArgs;
			std::vector<std::wstring> Strings;
			std::vector<int64_t> Integers;
			std::vector<double> Doubles;
			std::unordered_map<std::wstring, std::function<JSON::Item(const std::vector<std::wstring>&)>> Function{
				{ L"ObjectId", [](const std::vector<std::wstring>& str) { return ObjectId(str[0]); } },
				{ L"Binary", [](const std::vector<std::wstring>& str) { return Binary(PVX::Decode::FromHex(str[0]), str.size()>1? std::stoi(str[1]): 0); } }
			};

			auto tmp = RemoveStrings(Json, Strings);
			RemoveFunctions(tmp, Functions, FunctionArgs);
			removeSpaces(tmp);
			removeBoolsAndNulls(tmp);
			removeNumbers(tmp, Doubles, Integers);

			std::vector<jsonStack> Output, Stack2;
			Output.reserve(tmp.size());
			Stack2.reserve(tmp.size());
			std::vector<wchar_t> Stack;
			Stack.reserve(tmp.size());
			int ItemCount = 0;
			int ints = 0, floats = 0, strings = 0, functions = 0;

			for (auto& t : tmp) {
				switch (t) {
					case '{': t = (wchar_t)Symbols::OpenCurly; continue;
					case '[': t = (wchar_t)Symbols::OpenSquare; continue;
					case ']': t = (wchar_t)Symbols::CloseSquare; continue;
					case '}': t = (wchar_t)Symbols::CloseCurly; continue;
					case ',': t = (wchar_t)Symbols::Comma; continue;
					case ':': t = (wchar_t)Symbols::Colon; continue;
					case '@': t = (wchar_t)Symbols::Function; continue;
				}
			}

			for (auto c : tmp) {
				char Empty = 0;
				switch (c) {
					case (wchar_t)Symbols::Quote: Output.emplace_back(0, 0, std::move(Strings[strings++])); ItemCount++; break;
					case (wchar_t)Symbols::Integer: Output.emplace_back(0, 0, std::move(Integers[ints++])); ItemCount++; break;
					case (wchar_t)Symbols::Float: Output.emplace_back(0, 0, std::move(Doubles[floats++])); ItemCount++; break;
					case (wchar_t)Symbols::True: Output.emplace_back(0, 0, true); ItemCount++; break;
					case (wchar_t)Symbols::False: Output.emplace_back(0, 0, false); ItemCount++; break;
					case (wchar_t)Symbols::Null: Output.emplace_back(0, 0, jsElementType::Null); ItemCount++;  break;

					case (wchar_t)Symbols::Function: {
						std::vector<std::wstring> args = PVX::Map(FunctionArgs[functions], [&](const std::wstring& a) {
							return (a[0] == (wchar_t)Symbols::Quote)? Strings[strings++] : a;
						});

						if (auto fnc = Function.find(Functions[functions++]); fnc !=Function.end())
							Output.emplace_back(0, 0, fnc->second(args));
						else
							Output.emplace_back(0, 0, jsElementType::Undefined);
						ItemCount++;  
						break;
					}

					case (wchar_t)Symbols::OpenCurly: Stack.push_back((wchar_t)Symbols::OpenCurly); ItemCount = 0; break;
					case (wchar_t)Symbols::OpenSquare: Stack.push_back((wchar_t)Symbols::OpenSquare); ItemCount = 0; break;
					case (wchar_t)Symbols::CloseSquare: {
						if (Stack.size() && Stack.back() == (wchar_t)Symbols::OpenSquare && !ItemCount) Empty = 1;
						while (Stack.size() && Stack.back() != (wchar_t)Symbols::OpenSquare) {
							Output.emplace_back(Stack.back(), 0);
							Stack.pop_back();
						}
						if (!Stack.size()||Stack.back()!=(wchar_t)Symbols::OpenSquare)
							return jsElementType::Undefined;
						Stack.pop_back();
						Output.emplace_back((char)Symbols::OpenSquare, Empty, jsElementType::Array);
						ItemCount++;
						break;
					}
					case (wchar_t)Symbols::CloseCurly: {
						if (Stack.size() && Stack.back() == (wchar_t)Symbols::OpenCurly && !ItemCount) Empty = 1;
						while (Stack.size() && Stack.back() != (wchar_t)Symbols::OpenCurly) {
							Output.emplace_back(Stack.back());
							Stack.pop_back();
						}
						if (!Stack.size() || Stack.back() != (wchar_t)Symbols::OpenCurly)
							return jsElementType::Undefined;
						Stack.pop_back();
						Output.emplace_back((wchar_t)Symbols::OpenCurly, Empty, jsElementType::Object);
						ItemCount++;
						break;
					}
					case (wchar_t)Symbols::Comma:
						while (Stack.size() && Stack.back() > (wchar_t)Symbols::Comma) {
							Output.emplace_back(Stack.back());
							Stack.pop_back();
						}
						Stack.push_back(c);
						break;
					case (wchar_t)Symbols::Colon:
						Stack.push_back(c);
						break;
				}
			}
			while (Stack.size()) {
				Output.emplace_back(Stack.back());
				//Output.push_back({ Stack.back() });
				Stack.pop_back();
			}

			for (auto& s : Output) {
				switch (s.op) {
					case 0:	Stack2.emplace_back(std::move(s)); break;
					case (wchar_t)Symbols::Colon:
					case (wchar_t)Symbols::Comma: {
						if (Stack2.size() >= 2) {
							s.Child = new jsonStack*[2]{
								new jsonStack(std::move(Stack2.back())),
								new jsonStack(std::move(Stack2[Stack2.size()-2]))
							};
							Stack2.resize(Stack2.size()-2);

							Stack2.emplace_back(std::move(s));
							break;
						}
						return jsElementType::Undefined;
					}
					case (wchar_t)Symbols::OpenCurly: {
						if (s.Empty) {
							Stack2.emplace_back(std::move(s));
							break;
						} else if (Stack2.size()) {
							MakeObject(s.val, &Stack2.back());

							Stack2.pop_back();
							Stack2.emplace_back(std::move(s));
							break;
						}
						return jsElementType::Undefined;
					}
					case (wchar_t)Symbols::OpenSquare: {
						if (s.Empty) {
							Stack2.emplace_back(std::move(s));
							break;
						} else if (Stack2.size()) {
							MakeArray(s.val, &Stack2.back());

							Stack2.pop_back();
							Stack2.emplace_back(std::move(s));
							break;
						}
						return jsElementType::Undefined;
					}
				}
			}
			if (Stack2.size() == 1) return Stack2[0].val;
			return jsElementType::Undefined;
		}

		namespace BSON {
			double Double(const unsigned char*& cur) {
				cur += 8;
				return *(double*)(cur-8);
			}
			size_t Size(const unsigned char*& cur) {
				cur += 4;
				return (*(int*)(cur-4)) - 4;
			}
			int Int(const unsigned char*& cur) {
				cur += 4;
				return *(int*)(cur-4);
			}
			int64_t Int64(const unsigned char*& cur) {
				cur += 8;
				return *(int64_t*)(cur-8);
			}
			int Byte(const unsigned char*& cur) {
				return *(cur++);
			}
			std::wstring String(const unsigned char*& cur) {
				auto len = std::strlen((const char*)cur);
				cur += len + 1;
				return PVX::Decode::UTF(cur - len - 1, len);
			}
			const char* String2(const unsigned char*& cur) {
				auto len = std::strlen((const char*)cur);
				cur += len + 1;
				return (const char*)(cur - len - 1);
			}
			const std::vector<unsigned char> ObjectId(const unsigned char*& cur) {
				std::vector<unsigned char> ret(12);
				memcpy(&ret[0], cur, 12);
				cur += 12;
				return ret;
			}

			std::wstring ReadString(const unsigned char*& cur) {
				Int(cur);
				return String(cur);
			}

			JSON::Item ReadBinary(const unsigned char*& cur) {
				auto sz = Int(cur);
				std::vector<unsigned char> ret(sz);
				int Type = *(cur++);
				memcpy_s(&ret[0], sz, cur, sz);
				cur += sz;
				return Binary(std::move(ret), Type);
			}

			Item ReadArray(const unsigned char*& cur);

			Item ReadObject(const unsigned char*& cur) {
				auto sz = Size(cur);
				const unsigned char* End = cur + sz;
				Item ret = jsElementType::Object;
				while (cur < End) {
					auto tp = Byte(cur);
					if (tp) {
						auto name = String(cur);
						switch (tp) {
							case 0x01: ret[name] = Double(cur); break;
							case 0x02: ret[name] = ReadString(cur); break;
							case 0x03: ret[name] = ReadObject(cur); break;
							case 0x04: ret[name] = ReadArray(cur); break;
							case 0x05: ret[name] = ReadBinary(cur); break;
							case 0x07: ret[name] = ObjectId(cur); break;
							case 0x08: ret[name] = (bool)Byte(cur); break;
							case 0x0A: ret[name] = nullptr; break;
							case 0x10: ret[name] = Int(cur); break;
							case 0x12: ret[name] = Int64(cur); break;
							default: cur = End; return ret;
						};
						ret[name].BsonType() = (BSON_Type)tp;
					}
				}
				return ret;
			}
			Item ReadArray(const unsigned char*& cur) {
				auto sz = Size(cur);
				const unsigned char* End = cur + sz;
				Item ret = jsElementType::Array;
				Item item = jsElementType::Array;
				while (cur < End) {
					auto tp = Byte(cur);
					if (tp) {
						auto index = atoi(String2(cur));
						if (ret.length()!=index) ret.getArray().resize(index);
						switch (tp) {
							case 0x00: return ret;
							case 0x01: item = Double(cur); break;
							case 0x02: item = ReadString(cur); break;
							case 0x03: item = ReadObject(cur); break;
							case 0x04: item = ReadArray(cur); break;
							case 0x07: item = ObjectId(cur); break;
							case 0x08: item = (bool)Byte(cur); break;
							case 0x0A: item = nullptr; break;
							case 0x10: item = Int(cur); break;
							case 0x12: item = Int64(cur); break;
							default: cur = End; return ret;
						};
						item.BsonType() = (BSON_Type)tp;
						ret.push(item);
					}
				}
				return ret;
			}
		}
		Item fromBSON(const std::vector<unsigned char>& Data) {
			const unsigned char* cur = &Data[0];
			return BSON::ReadObject(cur);
		}
		JSON::Item fromBSON(const std::vector<unsigned char>& Data, size_t& Cursor) {
			const unsigned char* cur = &Data[Cursor];
			auto ret = BSON::ReadObject(cur);
			Cursor = cur - &Data[0];
			return ret;
		}
		Item fromBSON(const std::wstring& Data) {
			return fromBSON(PVX::IO::ReadBinary(Data.c_str()));
		}

		std::vector<unsigned char> ToBSON(const Item& obj) {
			std::vector<unsigned char> ret;
			ToBSON(obj, ret);
			return ret;
		}

		auto AppendBytes(std::vector<unsigned char>& Data, size_t Count) {
			auto cur = Data.size();
			Data.resize(cur + Count);
			return cur;
		}
		int AppendString(std::vector<unsigned char>& Data, const std::wstring& str) {
			auto len = (int)PVX::Encode::UTF_Length(str) + 1;
			PVX::Encode::UTF(&Data[AppendBytes(Data, len)], str.c_str());
			return len;
		}
		size_t AppendString(std::vector<unsigned char>& Data, const std::string& str) {
			auto cur = AppendBytes(Data, str.size() + 1);
			memcpy(&Data[cur], str.data(), str.size());
			return str.size();
		}
		size_t AppendData(std::vector<unsigned char>& Data, const std::vector<unsigned char>& str) {
			auto cur = AppendBytes(Data, str.size());
			memcpy(&Data[cur], str.data(), str.size());
			return str.size();
		}
		template<typename T>
		void Append(std::vector<unsigned char>& Data, T Prim) {
			auto cur = Data.size();
			Data.resize(cur + sizeof(T));
			(*(T*)&Data[cur]) = Prim;
		}

		void AppendStringObject(std::vector<unsigned char>& Data, const std::wstring& str) {
			auto SizeIndex = AppendBytes(Data, 4);
			AppendString(Data, str);
			(*(int*)&Data[SizeIndex]) = int(Data.size() - SizeIndex - 4);
		}

		void AppendObject(std::vector<unsigned char>& Data, const Item& obj) {
			auto SizeIndex = AppendBytes(Data, 4);
			auto& o = obj.getObject();
			for (auto& [n, v]: o) {
				if (v().BsonType() != JSON::BSON_Type::Undefined) {
					Append(Data, (unsigned char)v().BsonType());
					AppendString(Data, n);
					ToBSON(v, Data);
				}
			}
			Append(Data, (char)0);
			(*(int*)&Data[SizeIndex]) = int(Data.size() - SizeIndex);
		}
		void AppendArray(std::vector<unsigned char>& Data, const Item& obj) {
			auto SizeIndex = AppendBytes(Data, 4);
			auto& o = obj.getArray();
			size_t index = 0;
			for (auto& v: o) {
				if (v.BsonType() != JSON::BSON_Type::Undefined) {
					Append(Data, (unsigned char)v.BsonType());
					AppendString(Data, std::to_string(index));
					ToBSON(v, Data);
				}
				index++;
			}
			Append(Data, (char)0);
			(*(int*)&Data[SizeIndex]) = int(Data.size() - SizeIndex);
		}
		void AppendBinary(std::vector<unsigned char>& Data, const std::vector<unsigned char>& bin, int Type) {
			Data.reserve(Data.size() + 5 + bin.size());
			Append(Data, (int32_t)bin.size());
			Append(Data, (char)Type);
			AppendData(Data, bin);
		}

		void ToBSON(const Item& obj, std::vector<unsigned char>& Data) {
			switch (obj.BsonType()) {
				case PVX::JSON::BSON_Type::Double:
					Append(Data, obj.Double());
					break;
				case PVX::JSON::BSON_Type::String:
					AppendStringObject(Data, obj.String());
					break;
				case PVX::JSON::BSON_Type::Object: 
					AppendObject(Data, obj);
					break;
				case PVX::JSON::BSON_Type::Array:
					AppendArray(Data, obj);
					break;
				case PVX::JSON::BSON_Type::Binary:
					AppendBinary(Data, obj.Binary(), obj.BinaryType());
					break;
				//case PVX::JSON::BSON_Type::Undefined:
				//	break;
				case PVX::JSON::BSON_Type::ObjectId:
					AppendData(Data, obj.Binary());
					break;
				case PVX::JSON::BSON_Type::Boolean:
					Append(Data, (char)(obj.Boolean() ? 1 : 0));
					break;
				case PVX::JSON::BSON_Type::DateTimeUTC:
					Append(Data, obj.Integer());
					break;
				//case PVX::JSON::BSON_Type::Null:
				//	break;
				case PVX::JSON::BSON_Type::Regex:
					break;
				case PVX::JSON::BSON_Type::DBPointer:
					break;
				case PVX::JSON::BSON_Type::Code:
					AppendStringObject(Data, obj.String());
					break;
				case PVX::JSON::BSON_Type::Symbol:
					AppendStringObject(Data, obj.String());
					break;
				case PVX::JSON::BSON_Type::Code_w_s:
					break;
				case PVX::JSON::BSON_Type::Int32:
					Append(Data, (int)obj.Integer());
					break;
				case PVX::JSON::BSON_Type::Timestamp:
					Append(Data, obj.Integer());
					break;
				case PVX::JSON::BSON_Type::Int64:
					Append(Data, obj.Integer());
					break;
				case PVX::JSON::BSON_Type::Decimal128:
					break;
				case PVX::JSON::BSON_Type::MinKey:
					break;
				case PVX::JSON::BSON_Type::MaxKey:
					break;
				default:
					break;
			}
		}
		JSON::Item ObjectId(const std::string_view& hexId) {
			JSON::Item ret = PVX::Decode::FromHex(hexId);
			ret.BsonType() = BSON_Type::ObjectId;
			return ret;
		}
		JSON::Item ObjectId(const std::wstring_view& hexId) {
			JSON::Item ret = PVX::Decode::FromHex(hexId);
			ret.BsonType() = BSON_Type::ObjectId;
			return ret;
		}
		JSON::Item Binary(const std::vector<uint8_t>& Data, int Type) {
			JSON::Item ret = Data;
			ret.BinaryType() = Type;
			return ret;
		}
	}
	namespace BSON {
		Item::Item(const std::initializer_list<std::pair<std::wstring, const BsonVariant&>>& list) {
			Type = (char)JSON::BSON_Type::Object;
			AppendBytes(4);
			for (const auto& [n, v] : list) {
				switch (v.Index()) {
					case 0: write((char)JSON::BSON_Type::String);  write2(n); write(v.get<0>()); break;
					case 1: write((char)JSON::BSON_Type::String);  write2(n); write(v.get<1>()); break;
					case 2: write((char)JSON::BSON_Type::Boolean); write2(n); write(v.get<2>()); break;
					case 3: write((char)JSON::BSON_Type::Int32);   write2(n); write(v.get<3>()); break;
					case 4: write((char)JSON::BSON_Type::Int64);   write2(n); write(v.get<4>()); break;
					case 5: write((char)JSON::BSON_Type::Double);  write2(n); write(v.get<5>()); break;
					default: write(v.get<6>()->Type);  write2(n); write(*v.get<6>()); break;
				}
			}
			write(char(0));
			*(int32_t*)&Data[0] = int32_t(Data.size());
		}
		Item::Item(const std::initializer_list<std::pair<std::string, const BsonVariant&>>& list) {
			Type = (char)JSON::BSON_Type::Object;
			AppendBytes(4);
			for (const auto& [n, v] : list) {
				switch (v.Index()) {
					case 0: write((char)JSON::BSON_Type::String);  write2(n); write(v.get<0>()); break;
					case 1: write((char)JSON::BSON_Type::String);  write2(n); write(v.get<1>()); break;
					case 2: write((char)JSON::BSON_Type::Boolean); write2(n); write(v.get<2>()); break;
					case 3: write((char)JSON::BSON_Type::Int32);   write2(n); write(v.get<3>()); break;
					case 4: write((char)JSON::BSON_Type::Int64);   write2(n); write(v.get<4>()); break;
					case 5: write((char)JSON::BSON_Type::Double);  write2(n); write(v.get<5>()); break;
					default: write(v.get<6>()->Type);  write2(n); write(*v.get<6>()); break;
				}
			}
			write(char(0));
			*(int32_t*)&Data[0] = int32_t(Data.size());
		}
		Item::Item(const std::initializer_list<BsonVariant>& list) {
			Type = (char)JSON::BSON_Type::Array;
			AppendBytes(4);
			int i = 0;
			for (const auto& v : list) {
				switch (v.Index()) {
					case 0: write((char)JSON::BSON_Type::String);  write2(std::to_string(i++)); write(v.get<0>()); break;
					case 1: write((char)JSON::BSON_Type::String);  write2(std::to_string(i++)); write(v.get<1>()); break;
					case 2: write((char)JSON::BSON_Type::Boolean); write2(std::to_string(i++)); write(v.get<2>()); break;
					case 3: write((char)JSON::BSON_Type::Int32);   write2(std::to_string(i++)); write(v.get<3>()); break;
					case 4: write((char)JSON::BSON_Type::Int64);   write2(std::to_string(i++)); write(v.get<4>()); break;
					case 5: write((char)JSON::BSON_Type::Double);  write2(std::to_string(i++)); write(v.get<5>()); break;
					default: write(v.get<6>()->Type); write2(std::to_string(i++)); write(*v.get<6>()); break;
				}
			}
			write(char(0));
			*(int32_t*)&Data[0] = int32_t(Data.size());
		}
	}
}