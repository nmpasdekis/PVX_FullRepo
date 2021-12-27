#include <PVX_V8.h>
#include <PVX_Encode.h>
#include <PVX.inl>

namespace PVX {
	namespace Javascript {

		int v8Value::NextId = 0;

		static std::map<int, std::function<PVX::JSON::Item(const std::vector<PVX::JSON::Item>&)>> v8Functions;
		static std::map<int, std::function<void(const v8::FunctionCallbackInfo<v8::Value>&)>> v8Functions2;
		static std::map<int, std::function<v8Value(std::vector<v8Value>&)>> v8Functions3;

		static std::wstring ToString(const v8::Handle<v8::Value> & val) {
			std::string x = *v8::String::Utf8Value(v8::Isolate::GetCurrent(), val);
			return PVX::Decode::UTF((unsigned char*)x.data(), (int)(x.size()));
		}
		static v8::Local<v8::String> ToString(const std::wstring & val) {
			return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), (char*)PVX::Encode::UTF0(val).data()).ToLocalChecked();
		}
		static v8::Local<v8::String> ToString(const std::string & val) {
			return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), val.c_str()).ToLocalChecked();
		}
		v8Value::Array::Array(const std::vector<v8Value>& arr) : Arr{ arr } {}

		v8Value::~v8Value() {
			if (!Ref) {
				if (onDelete)onDelete();
				v8Functions.erase(Id);
				v8Functions2.erase(Id);
				v8Functions3.erase(Id);
			}
		}

		//v8Value::v8Value(const v8Value& v) : v8Data{ v.v8Data }, Parent{ v.Parent }, StringIndex{ v.StringIndex }, Index{ v.Index } {}
		v8Value::v8Value() : v8Data{ v8::Undefined(v8::Isolate::GetCurrent()) } {}
		v8Value::v8Value(const v8::Handle<v8::Value> & v) : v8Data{ v } {}
		v8Value::v8Value(const Array & a) : v8Data{ v8::Array::New(v8::Isolate::GetCurrent(), (int)a.Arr.size()) } {
			const auto & arr = v8Data.As<v8::Array>();
			for (auto i = 0; i < a.Arr.size(); i++)
				arr->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), i, a.Arr[i].GetValue());
		}
		v8Value::v8Value(const v8::Local<v8::Value> & v, const v8::Local<v8::Value> & Parent, const std::wstring & Index) : v8Data{ v }, Parent{ Parent }, StringIndex{ ToString(Index) }{}
		v8Value::v8Value(const v8::Local<v8::Value> & v, const v8::Local<v8::Value> & Parent, const v8::Local<v8::String> & Index) : v8Data{ v }, Parent{ Parent }, StringIndex{ Index }{}
		v8Value::v8Value(const v8::Local<v8::Value> & v, const v8::Local<v8::Value> & Parent, const int & Index) : v8Data{ v }, Parent{ Parent }, Index{ Index }{}
		v8Value::v8Value(const nullptr_t& v) : v8Data{ v8::Null(v8::Isolate::GetCurrent()) } {}
		v8Value::v8Value(const int& v) : v8Data{ v8::Int32::New(v8::Isolate::GetCurrent(), v) } {}
		v8Value::v8Value(const int64_t& v) : v8Data{ v8::BigInt::New(v8::Isolate::GetCurrent(), v) } {}
		v8Value::v8Value(const bool& v) : v8Data{ v8::Boolean::New(v8::Isolate::GetCurrent(), v) } {}
		v8Value::v8Value(const double& v) : v8Data{ v8::Number::New(v8::Isolate::GetCurrent(), v) } {}
		v8Value::v8Value(const std::wstring& v) : v8Data{ ToString(v) } {}
		v8Value::v8Value(const std::string & v) : v8Data{ ToString(v) } {}
		v8Value::v8Value(const wchar_t * v) : v8Data{ ToString(v) } {}
		v8Value::v8Value(const Type & v) {
			switch (v) {
			case Type::Object: v8Data = v8::Object::New(v8::Isolate::GetCurrent()); break;
			case Type::Array: v8Data = v8::Array::New(v8::Isolate::GetCurrent()); break;
			case Type::Null: v8Data = v8::Null(v8::Isolate::GetCurrent()); break;
			default: v8Data = v8::Undefined(v8::Isolate::GetCurrent()); break;
			}
		}

		v8Value::v8Value(std::function<PVX::JSON::Item(const std::vector<PVX::JSON::Item>&)> clb) { Function(clb); }
		v8Value::v8Value(std::function<v8Value(std::vector<v8Value>&)> clb) { Function(clb); }
		v8Value::v8Value(std::function<void(const v8::FunctionCallbackInfo<v8::Value>&)> clb) { Function(clb); }
		v8Value::v8Value(void(*clb)(const v8::FunctionCallbackInfo<v8::Value>&)) {
			Function(clb);
		}
		v8Value::v8Value(std::function<v8Value()> clb) {
			auto fnc = [clb](std::vector<v8Value>& Params) -> v8Value {
				return clb;
			};
			Function(fnc);
		}



		//v8Value::v8Value(const std::map<std::wstring, v8Value>& v) : v8Data{ v8::Object::New(v8::Isolate::GetCurrent()) } {
		//	const auto & obj = v8Data.As<v8::Object>();
		//	for (auto &[Name, Value] : v) obj->Set(ToString(Name), Value.GetValue());
		//}

		//v8Value::v8Value(const std::initializer_list<std::pair<std::wstring, v8Value>>& v) : v8Data{ v8::Object::New(v8::Isolate::GetCurrent()) } {
		//	const auto & obj = v8Data.As<v8::Object>();
		//	for (auto& [Name, Value] : v) //obj->Set(ToString(Name), Value.GetValue());
		//		(*this)[Name] = Value;
		//}

		bool v8Value::Contains(const std::wstring & Member) {
			return v8Data.As<v8::Object>()->Has(v8::Isolate::GetCurrent()->GetCurrentContext(), ToString(Member)).ToChecked();
		}
		void v8Value::forEach(std::function<void(const std::wstring&, const v8Value&)> fnc) {
			auto names = keys();
			for (auto & n : names) fnc(n, (*this)[n]);
		}
		void v8Value::forEach(std::function<void(int, const v8Value&)> fnc) {
			int Count = Length();
			for (auto i = 0; i < Count; i++) fnc(i, (*this)[i]);
		}
		void v8Value::forEach(std::function<void(const v8Value&)> fnc) {
			if (v8Data->IsArray()) {
				int Count = Length();
				for (auto i = 0; i < Count; i++) 
					fnc((*this)[i]);
			}
			else if (v8Data->IsObject()) {
				auto names = keys();
				for (auto & n : names)fnc((*this)[n]);
			}
		}
		v8Value v8Value::map(std::function<v8Value(const v8Value&)> fnc) {
			int len = Length();
			auto iso = v8::Isolate::GetCurrent();
			auto context = iso->GetCurrentContext();
			auto ret = v8::Array::New(iso, len);
			auto This = v8Data.As<v8::Array>();
			for (auto i = 0; i < len; i++) {
				ret->Set(context, i, fnc(v8Value(This->Get(context, i).ToLocalChecked())).GetValue());
			}
			return v8Value{ ret };
		}
		v8Value v8Value::filter(std::function<bool(const v8Value&)> fnc) {
			int len = Length();
			auto iso = v8::Isolate::GetCurrent();
			auto context = iso->GetCurrentContext();
			auto ret = v8::Array::New(iso, len);
			auto This = v8Data.As<v8::Array>();
			for (auto i = 0; i < len; i++) {
				auto x = This->Get(context, i).ToLocalChecked();
				if(fnc(v8Value(x))) ret->Set(context, i, x);
			}
			return v8Value{ ret };
		}

		v8Value& v8Value::SetValue(const v8::Local<v8::Value>& v) {
			v8Data = v;
			if (!Parent.IsEmpty()) {
				if (StringIndex.IsEmpty())
					Parent.As<v8::Array>()->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), Index, v8Data);
				else
					Parent.As<v8::Object>()->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), StringIndex, v8Data);
			}
			return *this;
		}

		v8Value& v8Value::SetValue(const v8Value & v) {
			SetValue(v.GetValue());
			onDelete = v.onDelete;
			ArrayChild.clear();
			for (auto& [k, v] : v.ArrayChild) {
				(*this)[k] = v;
				//ArrayChild[k] = v;
				//ArrayChild.insert({ k, v8Value{v} });
			}
			ObjectChild.clear();
			for (auto& [k, v] : v.ObjectChild) {
				(*this)[k] = v;
				//ObjectChild[k] = v;
				//ObjectChild.insert({ k, v8Value{v} });
			}
			Id = v.Id;
			Ref = v.Ref;
			return (*this);
		}

		v8Value& v8Value::operator=(const v8::Local<v8::Value>& v) { return SetValue(v); }
		v8Value& v8Value::operator=(const v8Value& v) {
			return SetValue(v); 
		}
		v8Value& v8Value::operator=(const nullptr_t& v) { return SetValue(v); }
		v8Value& v8Value::operator=(const bool& v) { return SetValue(v); }
		v8Value& v8Value::operator=(const double& v) { return SetValue(v); }
		v8Value& v8Value::operator=(const int& v) { return SetValue(v); }
		v8Value& v8Value::operator=(const std::wstring& v) { return SetValue(v); }
		v8Value& v8Value::operator=(const wchar_t * v) { return SetValue(std::wstring(v)); }

		v8Value& v8Value::operator=(void(*v)(const v8::FunctionCallbackInfo<v8::Value>&)) { return SetValue(v); }


		bool v8Value::operator!() {
			return !(IsNullOrUndefined()||(IsZero()));
		}

		v8Value& v8Value::operator&&(v8Value& v) {
			return !(*this) ? *this : v;
		}
		v8Value& v8Value::operator||(v8Value& v) {
			return !(*this) ?v : *this;
		}


		//v8Value v8Value::operator()(const std::vector<v8Value>& Params) {
		//	return v8Data.As<v8::Function>()->Call(
		//		v8::Isolate::GetCurrent()->GetCurrentContext(), 
		//		Parent, 
		//		(int)Params.size(), 
		//		Params.size() ? PVX::Map(Params, [](const v8Value& p) { return p.GetValue(); }).data() : nullptr).ToLocalChecked();
		//}

		std::vector<std::wstring> v8Value::keys() {
			auto context = v8::Isolate::GetCurrent()->GetCurrentContext();
			auto n = v8Data.As<v8::Object>()->GetPropertyNames(context).ToLocalChecked();
			int Count = n->Length();
			std::vector<std::wstring> ret;
			ret.reserve(Count);
			for (auto i = 0; i < Count; i++) ret.push_back(ToString(n->Get(context, i).ToLocalChecked()));
			return ret;
		}
		int v8Value::Length() {
			return v8Data.As<v8::Array>()->Length();
		}

		v8::Local<v8::Value> v8Value::GetValue() const { return v8Data; }

		v8Value::Type v8Value::GetType() {
			return Type::Undefined;
		}

		bool v8Value::IsNull() const { return v8Data->IsNull(); }
		bool v8Value::IsUndefined() const { return v8Data->IsUndefined(); }
		bool v8Value::IsNullOrUndefined() const { return v8Data->IsUndefined() || v8Data->IsNull(); }
		bool v8Value::IsArray() const {
			return v8Data->IsArray();
		}
		bool v8Value::IsObject() const {
			return v8Data->IsObject() && !(v8Data->IsArray() || v8Data->IsFunction() || v8Data->IsSet());
		}
		bool v8Value::IsEmpty() const {
			if (IsNullOrUndefined()) return true;
			if (v8Data->IsArray()) return v8Data.As<v8::Array>()->Length() == 0;
			else if (v8Data->IsObject() && !(v8Data->IsFunction() || v8Data->IsSet())) return v8Data.As<v8::Object>()->GetPropertyNames(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked()->Length() == 0;
			return false;
		}
		bool v8Value::IsZero() const {
			if (IsNullOrUndefined()) return true;
			if (v8Data->IsInt32()) return v8Data.As<v8::Int32>()->Value();
			if (v8Data->IsNumber()) return v8Data.As<v8::Number>()->Value();
			if (v8Data->IsBoolean()) return v8Data.As<v8::Boolean>()->Value();
			return false;
		}
		bool v8Value::Boolean() const { return v8Data.As<v8::Boolean>()->Value(); }
		double v8Value::Double() const { return v8Data.As<v8::Number>()->Value(); }
		int v8Value::Integer() const { return v8Data.As<v8::Int32>()->Value(); }
		int64_t v8Value::Integer64() const { return v8Data.As<v8::BigInt>()->Int64Value(); }
		v8Value v8Value::Call(const std::vector<v8Value>& args) {
			auto params = PVX::Map(args, [](const v8Value& it) { return it.GetValue(); });
			auto cntx = v8::Isolate::GetCurrent()->GetCurrentContext();
			return v8Data.As<v8::Function>()->Call(
				cntx,
				cntx->Global(),
				(int)params.size(),
				(params.size() ? &params[0] : nullptr)).ToLocalChecked();
		}
		v8Value v8Value::Call(v8Value & This, const std::vector<v8Value>& args) {
			auto params = PVX::Map(args, [](const v8Value& it) { return it.GetValue(); });
			return v8Data.As<v8::Function>()->Call(
				v8::Isolate::GetCurrent()->GetCurrentContext(),
				This.GetValue(),
				(int)params.size(),
				(params.size() ? &params[0] : nullptr)).ToLocalChecked();
		}
		std::wstring v8Value::String() const { return ToString(v8Data); }
		v8Value& v8Value::operator[](int Index) {
			if (auto ch = ArrayChild.find(Index); ch!=ArrayChild.end())
				return ch->second;
			ArrayChild.insert({ Index, { v8Data.As<v8::Array>()->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), Index).ToLocalChecked(), v8Data, Index } });
			return ArrayChild[Index];
		}
		v8Value& v8Value::operator[](const std::wstring & Index) {
			if (auto ch = ObjectChild.find(Index); ch!=ObjectChild.end())
				return ch->second;
			ObjectChild.insert({ Index, { v8Data.As<v8::Object>()->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), ToString(Index)).ToLocalChecked(), v8Data, Index } });
			return ObjectChild[Index];
		}
		v8Value& v8Value::operator[](const std::string & index) {
			auto Index = PVX::Encode::ToString(index);
			if (auto ch = ObjectChild.find(Index); ch!=ObjectChild.end())
				return ch->second;
			ObjectChild.insert({ Index, { v8Data.As<v8::Object>()->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), ToString(index)).ToLocalChecked(), v8Data, Index } });
			return ObjectChild[Index];
		}

		PVX::JSON::Item v8Value::ToJson() {
			return PVX::ToJson(GetValue());
		}
#define v8Undef v8::Undefined(v8::Isolate::GetCurrent())
		static v8::Local<v8::Value> Reduce(v8::Local<v8::Value> v) {
			auto anIsolate = v8::Isolate::GetCurrent();
			auto context = anIsolate->GetCurrentContext();

			if (v->IsNull()) return v8Undef;
			else if (v->IsArray()){
				auto arr = v.As<v8::Array>();
				int len = arr->Length();
				if(!len) return v8Undef;

				auto ret = v8::Array::New(anIsolate);
				int count = 0;
				for (int i = 0; i < len; i++) {
					auto item = Reduce(arr->Get(context, i).ToLocalChecked());
					count += item->IsUndefined() ? 1 : 0;
					ret->Set(context, i, item);
				}
				if (count == len)return v8Undef;
				return ret;
			} else if (v->IsObject() && !(v->IsFunction() || v->IsSet())) {
				auto obj = v.As<v8::Object>();
				auto names = obj->GetOwnPropertyNames(context).ToLocalChecked();

				int len = names->Length();

				if (!len) return v8Undef;
				auto ret = v8::Object::New(anIsolate);
				int count = 0;
				for (int i = 0; i < len; i++) {
					auto index = names->Get(context, i).ToLocalChecked();
					auto item = Reduce(obj->Get(context, index).ToLocalChecked());
					if (!item->IsUndefined()) {
						ret->Set(context, index, item);
						count++;
					}
				}
				if (count) return ret;
				return v8Undef;
			}
			return v;
		}
		v8Value v8Value::Reduce() {
			return PVX::Javascript::Reduce(v8Data);
		}

		static void PVX_BSON_Callback(const v8::FunctionCallbackInfo<v8::Value>& args) {}

		static void PVX_ObjectId_Callback(const v8::FunctionCallbackInfo<v8::Value>& args) {
			using namespace PVX::Javascript;
			using namespace std::string_literals;
			auto str = v8Value(args.Data())[L"Value"].String();
			args.GetReturnValue().Set<v8::Value>(v8Value(L"ObjectId("s + str + L")").GetValue());
		}

		v8::Local<v8::Value> v8Value::FromJson(const PVX::JSON::Item & Val) {
			auto cur = v8::Isolate::GetCurrent();
			switch (Val.BsonType()) {
				case JSON::BSON_Type::Int32: return v8::Integer::New(cur, (int32_t)Val.Integer());
				case JSON::BSON_Type::Int64: return v8::BigInt::New(cur, Val.Integer());
				case JSON::BSON_Type::Double: return v8::Number::New(cur, Val.Double());
				case JSON::BSON_Type::Null: return v8::Null(cur);
				case JSON::BSON_Type::String: return ToString(Val.String());
				case JSON::BSON_Type::Boolean: return v8::Boolean::New(cur, Val.Boolean());
				case JSON::BSON_Type::Array: {
					v8::Local<v8::Array> ret = v8::Array::New(cur, Val.length());
					auto context = v8::Isolate::GetCurrent()->GetCurrentContext();
					for (auto i = 0; i < Val.length(); i++)
						ret->Set(context, i, FromJson(Val[i]));
					return ret;
				}
				case JSON::BSON_Type::Object: {
					v8::Local<v8::Object> ret = v8::Object::New(cur);
					auto context = v8::Isolate::GetCurrent()->GetCurrentContext();
					Val.eachInObject([&ret, &context](const std::wstring& Name, const JSON::Item& Value) {
						ret->Set(context, ToString(Name), FromJson(Value));
					});
					return ret;
				}
				case JSON::BSON_Type::ObjectId: {
					auto context = v8::Isolate::GetCurrent()->GetCurrentContext();
					auto data = v8Value{
						{ L"Type", v8Value(int(JSON::BSON_Type::ObjectId)) },
						{ L"Value", v8Value(ToString(PVX::Encode::ToHex(Val.Value.Binary()))) }
					};
					auto ret = v8::FunctionTemplate::New(cur, PVX_BSON_Callback)->GetFunction(cur->GetCurrentContext()).ToLocalChecked();
					ret->SetName(ToString("BSON::ObjectId"));
					ret.As<v8::Object>()->Set(context, ToString("toJSON"), v8::FunctionTemplate::New(cur, PVX_ObjectId_Callback, data.GetValue())->GetFunction(cur->GetCurrentContext()).ToLocalChecked());
					return ret;
				}
			}
			return v8::Undefined(cur);
		}
		bool v8Value::Has(const std::wstring & Name) {
			return v8Data.As<v8::Object>()->Has(v8::Isolate::GetCurrent()->GetCurrentContext(), ToString(Name)).ToChecked();
		}

		static void PVX_FunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
			std::vector<PVX::JSON::Item> Params;
			for (int i = 0; i < args.Length(); i++)
				Params.push_back(PVX::ToJson(args[i]));
			while (Params.size()&&Params.back().IsUndefined()) Params.pop_back();

			auto ret = v8Functions[args.Data().As<v8::Int32>()->Value()](Params);
			args.GetReturnValue().Set<v8::Value>(v8Value::FromJson(ret));
		}

		static void PVX_FunctionCallback3(const v8::FunctionCallbackInfo<v8::Value>& args) {
			std::vector<v8Value> Params;
			for (int i = 0; i < args.Length(); i++) Params.push_back(args[i]);
			while (Params.size()&&Params.back().IsUndefined()) Params.pop_back();

			auto FuncId = args.Data().As<v8::Int32>()->Value();
			auto ret = v8Functions3[FuncId](Params);
			args.GetReturnValue().Set<v8::Value>(ret.GetValue());
		}

		static void PVX_FunctionCallback2(const v8::FunctionCallbackInfo<v8::Value>& args) {
			v8Functions2[args.Data().As<v8::Int32>()->Value()](args);
		}

		v8Value External(void* Data) {
			return (v8::Local<v8::Value>)v8::BigInt::New(v8::Isolate::GetCurrent(), int64_t(Data));
		}

		v8Value& v8Value::Function(std::function<PVX::JSON::Item(const std::vector<PVX::JSON::Item>&)> clb) {
			auto Isolate = v8::Isolate::GetCurrent();
			auto nfc = v8::FunctionTemplate::New(Isolate, PVX_FunctionCallback, v8::Int32::New(Isolate, (int32_t)Id));
			v8Functions[Id] = clb;
			auto fnc = nfc->GetFunction(Isolate->GetCurrentContext()).ToLocalChecked();
			v8Data = fnc;
			return *this;
		}

		v8Value& v8Value::Function(std::function<v8Value(std::vector<v8Value>&)> clb) {
			auto Isolate = v8::Isolate::GetCurrent();
			auto nfc = v8::FunctionTemplate::New(Isolate, PVX_FunctionCallback3, v8::Int32::New(Isolate, (int32_t)Id));
			v8Functions3[Id] = clb;
			auto fnc = nfc->GetFunction(Isolate->GetCurrentContext()).ToLocalChecked();
			v8Data = fnc;
			return *this;
		}

		v8Value& v8Value::Function(std::function<void(const v8::FunctionCallbackInfo<v8::Value>&)> clb) {
			auto Isolate = v8::Isolate::GetCurrent();
			auto tmp = clb.target<void(*)(const v8::FunctionCallbackInfo<v8::Value>&)>();
			v8::Local<v8::Function> fnc;
			if (tmp) {
				auto nfc = v8::FunctionTemplate::New(Isolate, *tmp);
				fnc = nfc->GetFunction(Isolate->GetCurrentContext()).ToLocalChecked();
			} else {
				auto nfc = v8::FunctionTemplate::New(Isolate, PVX_FunctionCallback2, v8::Int32::New(Isolate, (int32_t)Id));
				v8Functions2[Id] = clb;
				fnc = nfc->GetFunction(Isolate->GetCurrentContext()).ToLocalChecked();
			}
			v8Data = fnc;
			return *this;
		}

		v8Value& v8Value::operator=(std::function<PVX::JSON::Item(const std::vector<PVX::JSON::Item>&)> clb) {
			return SetValue(clb);
		}
		v8Value& v8Value::operator=(std::function<v8Value(std::vector<v8Value>&)> clb) {
			return SetValue(clb);
		}
		v8Value& v8Value::operator=(std::function<void(const v8::FunctionCallbackInfo<v8::Value>&)> clb) {
			return SetValue(clb);
		}
		v8Value& v8Value::operator=(std::function<v8Value()> clb) {
			return SetValue({ 
				[clb](std::vector<v8Value>& Params) -> v8Value {
					return clb();
				} 
			});
		}


		//void v8Value::SetNull(){}
		//void v8Value::Double(double){}
		//void v8Value::Integer(int64_t){}
		//void v8Value::String(std::wstring){}
	}
}