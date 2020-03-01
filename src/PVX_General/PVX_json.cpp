#include<PVX_json.h>
#include<PVX_Encode.h>
#include<sstream>
#include <stdio.h>
#include <PVX_String.h>
#include <PVX_Encode.h>
#include <PVX_File.h>
#include <string_view>
#include <PVX_Regex.h>

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
		static void WriteNumber(FILE* f, size_t n) {
			fwrite(&n, 4, 1, f);
		}
		static void WriteString(FILE* f, const std::wstring& s) {
			WriteNumber(f, s.size());
			fwrite(&s[0], sizeof(wchar_t), s.size(), f);
		}
		void Item::WriteBin(void* f) {
			WriteNumber((FILE*)f, (size_t)Value.GetType());
			switch (Value.GetType()) {
				case JSON::jsElementType::Integer: fwrite(&Integer(), sizeof(long long), 1, (FILE*)f); break;
				case JSON::jsElementType::Float: fwrite(&Double(), sizeof(double), 1, (FILE*)f); break;
				case JSON::jsElementType::String: WriteString((FILE*)f, String()); break;
				case JSON::jsElementType::Array:
				{
					auto& Array = Value.Array();
					WriteNumber((FILE*)f, Array.size());
					for (auto& i : Array)
						i.WriteBin(f);
					break;
				}
				case JSON::jsElementType::Object:
				{
					auto& Object = Value.Object();
					WriteNumber((FILE*)f, Object.size());
					for (auto& [n, v] : Object) {
						if (v.Value.GetType()!= JSON::jsElementType::Undefined && v.Value.GetType()!=JSON::jsElementType::Null)
							WriteString((FILE*)f, n);
						v.WriteBin(f);
					}
					break;
				}

			};
		}
		int Item::SaveBinary(const wchar_t* Filename) {
			FILE* fout;
			if (!_wfopen_s(&fout, Filename, L"wb")) {
				WriteBin(fout);
				fclose(fout);
			}
			return 0;
		}

		int Item::SaveBinary(const char* Filename) {
			FILE* fout;
			if (!fopen_s(&fout, Filename, "wb") && fout) {
				WriteBin(fout);
				fclose(fout);
			}
			return 0;
		}
		static double ReadDouble(FILE* f) {
			double ret;
			fread(&ret, sizeof(double), 1, f);
			return ret;
		}
		static size_t ReadInt(FILE* f) {
			size_t ret = 0;
			fread(&ret, 4, 1, f);
			return ret;
		}
		static long long ReadLong(FILE* f) {
			long long ret = 0;
			fread(&ret, sizeof(long long), 1, f);
			return ret;
		}
		static std::wstring ReadString(FILE* f) {
			size_t tmp = 0;
			fread(&tmp, 4, 1, f);
			std::wstring ret;
			ret.resize(tmp);
			fread(&ret[0], sizeof(wchar_t), tmp, f);
			return ret;
		}
		Item Item::ReadBin(void* f) {
			JSON::jsElementType type = (JSON::jsElementType)ReadInt((FILE*)f);
			size_t count;
			switch (type) {
				case JSON::jsElementType::Integer:
					return ReadLong((FILE*)f);
				case JSON::jsElementType::Float:
					return ReadDouble((FILE*)f);
				case JSON::jsElementType::String:
					return ReadString((FILE*)f);
				case JSON::jsElementType::Array:
				{
					count = ReadInt((FILE*)f);
					Item ret = JSON::jsElementType::Array;
					auto& Array = ret.Value.Array();
					for (auto i = 0; i < count; i++) Array.push_back(ReadBin(f));
					return ret;
				};
				case JSON::jsElementType::Object:
				{
					count = ReadInt((FILE*)f);
					Item ret = JSON::jsElementType::Object;
					auto& Object = ret.Value.Object();
					for (auto i = 0; i < count; i++) { auto name = ReadString((FILE*)f); Object[name] = ReadBin(f); }
					return ret;
				};
			};
			return type;
		}
		Item Item::ReadBinary(const char* Filename) {
			FILE* fin;
			if (!fopen_s(&fin, Filename, "rb")) {
				return ReadBin(fin);
			}
			return jsElementType::Undefined;
		}
		Item Item::ReadBinary(const wchar_t* Filename) {
			FILE* fin;
			if (!_wfopen_s(&fin, Filename, L"rb")) {
				return ReadBin(fin);
			}
			return jsElementType::Undefined;
		}

		Item::Item(const jsArray& its) {
			std::vector<Item> Array;
			for (auto& it : its.itms) {
				Array.push_back(it);
			}
			Value = Array;
		}

		static std::wstring wstr(const std::string& str) {
			std::wstring ret; ret.resize(str.size());
			memcpy(&ret[0], &str[0], str.size());
			return ret;
		}
		Item& Item::operator=(const jsElementType tp) {
			switch (tp) {
				case jsElementType::Undefined: Value = false; break;
				case jsElementType::Null: Value = false; break;
				case jsElementType::Integer: Value = 0; break;
				case jsElementType::Float: Value = 0.0; break;
				case jsElementType::String: Value = L""; break;
				case jsElementType::Array: Value = std::vector<Item>(); break;
				case jsElementType::Object: Value = std::unordered_map<std::wstring, Item>(); break;
				case jsElementType::Boolean: Value = false; break;
			}
			return *this;
		}
		const Item& Item::operator||(const Item& item) const {
			if (!IsEmpty()) return *this;
			return item;
		}
		const Item& Item::operator&&(const Item& item) const {
			if (IsEmpty()) return false;
			return item;
		}
		Item& Item::operator=(const nullptr_t& n) {
			Value = nullptr;
			return *this;
		}
		Item& Item::operator=(const int v) {
			Value = (long long)v;
			return *this;
		}
		Item& Item::operator=(const long long v) {
			Value = (long long)v;
			return *this;
		}
		Item& Item::operator=(const float v) {
			Value = (double)v;
			return *this;
		}
		Item& Item::operator=(const double v) {
			Value = v;
			return *this;
		}
		Item& Item::operator=(const bool v) {
			Value = v;
			return *this;
		}
		Item& Item::operator=(const std::wstring& v) {
			Value = v;
			return *this;
		}

		Item& Item::operator=(const std::string& s) {
			std::wstring String;
			String.reserve(s.size());
			for (auto c : s) String.push_back(c);
			Value = String;
			return *this;
		}
		Item& Item::operator=(const wchar_t* v) {
			Value = std::wstring(v);
			return *this;
		}

		Item& Item::operator=(const char* s2) {
			std::string s = s2;
			std::wstring String;
			String.reserve(s.size());
			for (auto c : s) String.push_back(c);
			Value = String;
			return *this;
		}

		Item& Item::operator=(const std::vector<unsigned char>& v) {
			std::string tmp = PVX::Encode::Base64(v);
			std::wstring String;
			String.reserve(tmp.size());
			for (auto c : tmp) String.push_back(c);
			Value = String;
			return *this;
		}


		Item& Item::operator[](const std::wstring& Name) {
			return Value.Object()[Name];
		}
		const Item& Item::operator[](const std::wstring& Name) const {
			return Value.Object().at(Name);
		}

		Item& Item::operator[](const std::string& Name) {
			std::wstring n;
			for (auto c : Name)n.push_back(c);
			return Value.Object()[n];
		}
		const Item& Item::operator[](const std::string& Name) const {
			std::wstring n;
			for (auto c : Name)n.push_back(c);
			return Value.Object().at(n);
		}

		Item& Item::operator[](int Index) {
			return Value.Array()[Index];
		}
		const Item& Item::operator[](int Index) const {
			return Value.Array()[Index];
		}

		Item Item::Get(const std::wstring& Name, const Item& Default) const {
			auto& Object = Value.Object();
			if (auto ret = Object.find(Name); ret!=Object.end())
				return ret->second;
			return Default;
		}

		const Item* Item::Has(const std::wstring& Name) const {
			auto& Object = Value.Object();
			if (auto ret = Object.find(Name); ret != Object.end())
				return &(ret->second);
			return nullptr;
		}

		Item& Item::operator<<(const std::wstring& s) {
			*this = parse(s.c_str());
			return *this;
		}

		void Item::push(const Item& it) {
			Value.Array().emplace_back(it);
		}

		Item Item::pop() {
			auto& Array = Value.Array();
			auto ret = Array.back();
			Array.pop_back();
			return ret;
		}

		int Item::length() const {
			return (int)Value.Array().size();
		}

		bool Item::IsNull() const {
			return Value.GetType() == jsElementType::Null;
		}
		bool Item::IsUndefined() const {
			return Value.GetType() == jsElementType::Undefined;
		}
		bool Item::IsNullOrUndefined() const {
			return Value.GetType() == jsElementType::Null || Value.GetType() == jsElementType::Undefined;
		}
		bool Item::IsEmpty()  const {
			auto Type = Value.GetType();
			return
				Type == jsElementType::Null ||
				Type == jsElementType::Undefined ||
				((Type == jsElementType::Integer || Type == jsElementType::Boolean || Type == jsElementType::Float) && !Integer()) ||
				(Type == jsElementType::String && !String().size()) ||
				(Type == jsElementType::Array && !Value.Array().size()) ||
				(Type == jsElementType::Object && !Value.Object().size());
		}

		std::vector<std::wstring> Item::Keys() const {
			std::vector<std::wstring> ret;
			for (auto& kv : Value.Object())
				ret.push_back(kv.first);
			return ret;
		}
		std::vector<PVX::JSON::Item> Item::Values() const {
			std::vector<PVX::JSON::Item> ret;
			for (auto& kv : Value.Object())
				ret.push_back(kv.second);
			return ret;
		}

		double Item::NumberSafeDouble() const {
			switch (Value.GetType()) {
				case jsElementType::Float: return Value.Double();
				case jsElementType::Integer: return (double)Value.Integer();
				case jsElementType::String: return _wtof(Value.String().c_str());
				default: 0.0;
			}
		}

		long long Item::NumberSafeInteger() const {
			switch (Value.GetType()) {
				case jsElementType::Float: return (long long)Value.Double();
				case jsElementType::Integer: return Value.Integer();
				case jsElementType::String: return _wtoi(Value.String().c_str());
				default: 0ll;
			}
		}

		std::vector<unsigned char> Item::Data() {
			return PVX::Decode::Base64(String());
		}

		std::wstring Item::GetString() const {
			if (Value.GetType()== jsElementType::String)
				return String();
			return stringify(*this);
		}

		void Item::Data(const std::vector<unsigned char>& d) {
			Value = PVX::Encode::ToString(PVX::Encode::Base64(d));
		}

		Item Item::map(std::function<Item(const Item&)> Convert) {
			if (Value.GetType() == JSON::jsElementType::Array) {
				Item ret = JSON::jsElementType::Array;
				for (auto& i : Value.Array()) {
					ret.push(Convert(i));
				}
				return ret;
			}
			return jsElementType::Undefined;
		}

		Item Item::map2(std::function<Item(const Item&, int Index)> Convert) {
			if (Value.GetType() == JSON::jsElementType::Array) {
				Item ret = JSON::jsElementType::Array;
				int Index = 0;
				for (auto& i : Value.Array()) {
					ret.push(Convert(i, Index++));
				}
				return ret;
			}
			return jsElementType::Undefined;
		}

		void Item::each(std::function<void(Item&)> Func) {
			if (Value.GetType() == JSON::jsElementType::Array) for (auto& i : Value.Array()) Func(i);
		}
		void Item::each(std::function<void(const Item&)> Func) const {
			if (Value.GetType() == JSON::jsElementType::Array) for (auto& i : Value.Array()) Func(i);
		}
		void Item::each2(std::function<void(Item&, int Index)> Func) {
			int Index = 0;
			if (Value.GetType() == JSON::jsElementType::Array) for (auto& i : Value.Array()) Func(i, Index++);
		}
		void Item::each2(std::function<void(const Item&, int Index)> Func) const {
			int Index = 0;
			if (Value.GetType() == JSON::jsElementType::Array) for (auto& i : Value.Array()) Func(i, Index++);
		}
		void Item::eachInObject(std::function<void(const std::wstring& Name, Item&)> Func) {
			if (Value.GetType()!=jsElementType::Object)return;
			for (auto& [Name, Value] : Value.Object()) Func(Name, Value);
		}
		void Item::eachInObject(std::function<void(const std::wstring& Name, const Item&)> Func) const {
			if (Value.GetType()!=jsElementType::Object)return;
			for (const auto& [Name, Value] : Value.Object()) Func(Name, Value);
		}
		Item Item::GroupBy(std::function<std::wstring(const Item&)> Func) {
			if (Value.GetType()!=jsElementType::Array) return jsElementType::Undefined;
			Item ret = jsElementType::Object;
			each([&](const Item& it) {
				std::wstring Name = Func(it);
				if (!ret.Has(Name)) ret[Name] = jsElementType::Array;
				ret[Name].push(it);
			});
			return ret;
		}

		Item Item::filter(std::function<int(const Item&)> Test) {
			if (Value.GetType() == JSON::jsElementType::Array) {
				Item ret = JSON::jsElementType::Array;
				for (auto& i : Value.Array()) {
					if (Test(i))
						ret.push(i);
				}
				return ret;
			}
			return jsElementType::Undefined;
		}
		Item Item::find(std::function<int(const Item&)> Test, size_t Start) {
			auto& Array = Value.Array();
			if (Value.GetType() == JSON::jsElementType::Array) for (auto i = Start; i < Array.size(); i++) if (Test(Array[i])) return Array[i];
			return jsElementType::Undefined;
		}

		int Item::findIndex(std::function<int(const Item&)> Test, size_t Start) {
			auto& Array = Value.Array();
			if (Value.GetType() == JSON::jsElementType::Array) for (auto i = Start; i < Array.size(); i++) if (Test(Array[i])) return i;
			return -1;
		}

		Item Item::sort(std::function<int(Item&, Item&)> Compare) {
			if (Value.GetType() == JSON::jsElementType::Array) {
				Item ret = Copy();
				std::sort(ret.Value.Array().begin(), ret.Value.Array().end(), Compare);
				return ret;
			}
			return jsElementType::Undefined;
		}

		Item Item::Copy() {
			Item ret = Value.GetType();
			ret.Value = Value;
			return ret;
		}

		Item Item::DeepCopy() {
			switch (Value.GetType()) {
				case PVX::JSON::jsElementType::Undefined:
				case PVX::JSON::jsElementType::Null:
				case PVX::JSON::jsElementType::Float:
				case PVX::JSON::jsElementType::Integer:
				case PVX::JSON::jsElementType::String:
				case PVX::JSON::jsElementType::Boolean:
					return (*this);
				case PVX::JSON::jsElementType::Array:
					return map([](auto x) { return x.DeepCopy(); });
					break;
				case PVX::JSON::jsElementType::Object: {
					Item ret = Value.GetType();
					for (auto& [k, v]: Value.Object()) {
						ret[k] = v.DeepCopy();
					}
					return ret;
				}
			}
			return PVX::JSON::jsElementType::Undefined;
		}
		Item Item::DeepReducedCopy() {
			switch (Value.GetType()) {
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
					for (auto& x: Value.Array()) {
						auto y = x.DeepReducedCopy();
						count += y.Type() == jsElementType::Undefined;
						ret.push(y);
					}
					if (count == length()) return jsElementType::Undefined;
					return ret;
				}
				case PVX::JSON::jsElementType::Object: {
					Item ret = jsElementType::Object;
					for (auto& [k, v]: Value.Object()) {
						auto x = v.DeepReducedCopy();
						if (x.Type() != jsElementType::Undefined)
							ret[k] = x;
					}
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
					for (auto& it : With.Value.Object())
						(*this)[it.first].Merge(it.second);
				} else if (this->Type() == JSON::jsElementType::Array) {
					for (auto& it : With.Value.Array())
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
					switch (obj.Value.Bson()) {
						case BSON_Type::ObjectId:
							return L"ObjectId(\"" + PVX::Encode::wToHex(obj.Value.Binary()) + L"\")";
					}
					return L"undefined";
				case jsElementType::Array:
					ret << "[";
					{
						auto& Array = obj.Value.Array();
						if (Array.size()) {
							size_t i = 0;
							while (i< Array.size() && Array[i].Type()==jsElementType::Undefined)i++;
							ret << Lvl1 << stringify(Array[i], level + 1, Format); i++;
							while (i< Array.size() && Array[i].Type()==jsElementType::Undefined)i++;
							for (; i < Array.size(); i++) {
								ret << "," << Lvl1 << stringify(Array[i], level + 1, Format);
								while (i< Array.size() && Array[i].Type()==jsElementType::Undefined)i++;
							}
						}
					}
					ret << Lvl << "]";
					return ret.str();
				case jsElementType::Object:
				{
					ret << "{";
					auto& Object = obj.Value.Object();
					auto iter = Object.begin();

					while (iter!=Object.end() && iter->second.Type() == jsElementType::Undefined) iter++;

					if (iter != Object.end()) {
						ret << Lvl1 << JsonString(iter->first) << colon << stringify(iter->second, level + 1, Format);
						++iter;
					}

					for (; iter != Object.end(); ++iter) {
						if (iter->second.Type() != jsElementType::Undefined)
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
		Item parse(const unsigned char* data, int size) {
			if (!size)return jsElementType::Undefined;
			return parse(Decode::UTF(data, size).c_str());
		}
		Item parse(const std::vector<unsigned char>& d) {
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




			//if (s->op == (char)Symbols::Colon) {
			//	obj.AddProperty(std::move(s->Child[1]->val.String()), std::move(s->Child[0]->val));
			//	//obj[std::move(s->Child[1]->val.String())] = std::move(s->Child[0]->val);
			//	delete s->Child[0];
			//	delete s->Child[1];
			//	delete s->Child;
			//	s->Child = nullptr;
			//} else if (s->op == (char)Symbols::Comma) {
			//	MakeObject(obj, s->Child[0]);
			//	MakeObject(obj, s->Child[1]);
			//} else obj = jsElementType::Undefined;
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
			//if (s->op == (char)Symbols::Comma) {
			//	MakeArray(obj, std::move(s->Child[1]));
			//	MakeArray(obj, std::move(s->Child[0]));

			//	delete s->Child[0];
			//	delete s->Child[1];
			//	delete s->Child;
			//	s->Child = nullptr;
			//} 
			//else {
			//	obj.push(s->val);
			//}
		}

		//struct jsonStack {
		//	int op;
		//	int Empty = 0;
		//	PVX::JSON::Item val;
		//	std::vector<jsonStack> Child;
		//	jsonStack(int op, int empty, PVX::JSON::Item&& val) : op{ op }, Empty{ empty }, val{ std::move(val) }{}
		//	jsonStack(int op, char empty) : op{ op }, Empty{ empty }{}
		//	jsonStack(int op) : op{ op } {
		//		Child.reserve(2);
		//	}
		//	jsonStack(const jsonStack&) = delete;
		//	jsonStack(jsonStack&& v) = default;
		//};

		//void MakeObject(Item& obj, jsonStack&& s) {
		//	if (s.op == (char)Symbols::Colon) {
		//		obj[s.Child[1].val.String()] = std::move(s.Child[0].val);
		//	} else if (s.op == (char)Symbols::Comma) {
		//		for (auto& c : s.Child)
		//			MakeObject(obj, std::move(c));
		//	} else obj = jsElementType::Undefined;
		//}
		//void MakeArray(Item& obj, jsonStack&& s) {
		//	if (s.op == (char)Symbols::Comma) {
		//		for (long long i = long long(s.Child.size()) - 1; i >= 0; i--)
		//			MakeArray(obj, std::move(s.Child[i]));
		//	} else obj.push(s.val);
		//}

		static std::wstring RemoveStrings2(const std::wstring& txt, std::vector<std::wstring>& Strings) {
			std::wstring ret;
			int Escape = 0;
			long long Start = -1;
			int Out = 1;
			for (long long i = 0; i < txt.size(); i++) {
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

		static std::wstring RemoveStrings(const std::wstring_view& text, std::vector<std::wstring>& Strings) {
			std::wstring Text = text.data();
			long long index = Text.find('"');
			while (index != -1) {
				long long end = Text.find('"', index + 1);
				while (end != -1 && Text[end - 1] == '\\') end = Text.find('"', end + 1);
				if (end == -1)
					return L"";

				Strings.push_back(Text.substr(index + 1, end - index - 1));
				Text = Text.replace(index, end - index + 1, L"\x01");
				index = Text.find('"', index + 1);
			}
			return std::move(Text);
		}

		long long FindNum(std::wstring& txt, long long& cur, long long start) {
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

		void removeNumbers(std::wstring& txt, std::vector<double>& Doubles, std::vector<long long>& Integers) {
			long long cur = 0;
			long long start = FindNum(txt, cur, 0);
			while (start != -1) {
				auto end = start + 1;
				while (txt[end]>=L'0' && txt[end]<=L'9') end++;
				if (txt[end]&&txt[end]==L'.') {
					end++;
					while (txt[end]>=L'0' && txt[end]<=L'9') end++;
					Doubles.push_back(_wtof(&txt[start]));
					txt[cur] = (wchar_t)Symbols::Float;
				} else {
					Integers.push_back(_wtoi64(&txt[start]));
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

		void RemoveFunctions(std::wstring& Text, std::vector<std::wstring>& Functions) {
			Text = PVX::Replace(Text, LR"regex(([$a-zA-Z_][$a-zA-Z_0-9]*)\(([^\)]*)\))regex", [&Functions](const std::wsmatch& m) {
				Functions.emplace_back(m[1].str());
				return L"@";
			});
		}

		Item parse(const std::wstring_view& Json) {
			std::vector<std::wstring> Strings;
			std::vector<long long> Integers;
			std::vector<double> Doubles;

			auto tmp = RemoveStrings(Json, Strings);
			removeSpaces(tmp);
			removeBoolsAndNulls(tmp);
			removeNumbers(tmp, Doubles, Integers);

			std::vector<jsonStack> Output, Stack2;
			Output.reserve(tmp.size());
			Stack2.reserve(tmp.size());
			std::vector<char> Stack;
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
				Output.push_back({ Stack.back() });
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
			std::vector<std::wstring> Strings;
			std::vector<long long> Integers;
			std::vector<double> Doubles;
			std::unordered_map<std::wstring, std::function<JSON::Item(const std::wstring&)>> Function{
				{ L"ObjectId", [](const std::wstring& str) { return ObjectId(str); } }
			};

			auto tmp = RemoveStrings(Json, Strings);
			RemoveFunctions(tmp, Functions);
			removeSpaces(tmp);
			removeBoolsAndNulls(tmp);
			removeNumbers(tmp, Doubles, Integers);

			std::vector<jsonStack> Output, Stack2;
			Output.reserve(tmp.size());
			Stack2.reserve(tmp.size());
			std::vector<char> Stack;
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
						auto& arg = Strings[strings++];
						if (auto fnc = Function.find(Functions[functions++]); fnc !=Function.end())
							Output.emplace_back(0, 0, fnc->second(arg));
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
				Output.push_back({ Stack.back() });
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
							//Stack2.back() = s;
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
			int Int64(const unsigned char*& cur) {
				cur += 8;
				return *(long long*)(cur-8);
			}
			int Byte(const unsigned char*& cur) {
				return *(cur++);
			}
			std::wstring String(const unsigned char*& cur) {
				auto len = strlen((const char*)cur);
				cur += len + 1;
				return PVX::Decode::UTF(cur - len - 1, len);
			}
			const char* String2(const unsigned char*& cur) {
				auto len = strlen((const char*)cur);
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
				auto sz = Int(cur);
				return String(cur);
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
							case 0x07: ret[name].Value = ObjectId(cur); break;
							case 0x08: ret[name] = (bool)Byte(cur); break;
							case 0x0A: ret[name] = nullptr; break;
							case 0x10: ret[name] = Int(cur); break;
							case 0x12: ret[name] = Int64(cur); break;
							default: cur = End; return ret;
						};
						ret[name].Value.Bson() = (BSON_Type)tp;
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
						item.Value.Bson() = (BSON_Type)tp;
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

		auto AppendBytes(std::vector<unsigned char>& Data, int Count) {
			auto cur = Data.size();
			Data.resize(cur + Count);
			return cur;
		}
		int AppendString(std::vector<unsigned char>& Data, const std::wstring& str) {
			auto len = (int)PVX::Encode::UTF_Length(str) + 1;
			PVX::Encode::UTF(&Data[AppendBytes(Data, len)], str.c_str());
			return len;
		}
		int AppendString(std::vector<unsigned char>& Data, const std::string& str) {
			auto cur = AppendBytes(Data, str.size() + 1);
			memcpy(&Data[cur], str.data(), str.size());
			return str.size();
		}
		int AppendData(std::vector<unsigned char>& Data, const std::vector<unsigned char>& str) {
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
			(*(int*)&Data[SizeIndex]) = Data.size() - SizeIndex - 4;
		}

		void AppendObject(std::vector<unsigned char>& Data, const Item& obj) {
			auto SizeIndex = AppendBytes(Data, 4);
			auto& o = obj.getObject();
			for (auto& [n, v]: o) {
				if (v.Value.Bson() != JSON::BSON_Type::Undefined) {
					Append(Data, (unsigned char)v.Value.Bson());
					AppendString(Data, n);
					ToBSON(v, Data);
				}
			}
			Append(Data, (char)0);
			(*(int*)&Data[SizeIndex]) = Data.size() - SizeIndex;
		}
		void AppendArray(std::vector<unsigned char>& Data, const Item& obj) {
			auto SizeIndex = AppendBytes(Data, 4);
			auto& o = obj.getArray();
			size_t index = 0;
			for (auto& v: o) {
				if (v.Value.Bson() != JSON::BSON_Type::Undefined) {
					Append(Data, (unsigned char)v.Value.Bson());
					AppendString(Data, std::to_string(index));
					ToBSON(v, Data);
				}
				index++;
			}
			Append(Data, (char)0);
			(*(int*)&Data[SizeIndex]) = Data.size() - SizeIndex;
		}

		void ToBSON(const Item& obj, std::vector<unsigned char>& Data) {
			switch (obj.Value.Bson()) {
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
					break;
				//case PVX::JSON::BSON_Type::Undefined:
				//	break;
				case PVX::JSON::BSON_Type::ObjectId:
					AppendData(Data, obj.Value.Binary());
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
			ret.Value.Bson() = BSON_Type::ObjectId;
			return ret;
		}
		JSON::Item ObjectId(const std::wstring_view& hexId) {
			JSON::Item ret = PVX::Decode::FromHex(hexId);
			ret.Value.Bson() = BSON_Type::ObjectId;
			return ret;
		}
	}
	namespace BSON {
		Item::Item(const std::initializer_list<std::pair<std::wstring, BsonVariant>>& list) {
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
					default: write(v.get<6>().Type);  write2(n); write(v.get<6>()); break;
				}
			}
			write(char(0));
			*(int32_t*)&Data[0] = Data.size();
		}
		Item::Item(const std::initializer_list<std::pair<std::string, BsonVariant>>& list) {
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
					default: write(v.get<6>().Type);  write2(n); write(v.get<6>()); break;
				}
			}
			write(char(0));
			*(int32_t*)&Data[0] = Data.size();
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
					default: write(v.get<6>().Type);  write2(std::to_string(i++)); write(v.get<6>()); break;
				}
			}
			write(char(0));
			*(int32_t*)&Data[0] = Data.size();
		}
	}
}