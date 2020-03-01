#define __PVX_V8_CPP__
#include <PVX_V8.h>
#include <libplatform/libplatform.h>
#include <stdio.h>
#include <PVX_Encode.h>
#include <PVX.inl>
#include <PVX_Regex.h>
#include <atomic>
#include <PVX_File.h>

#pragma comment (lib, "PVX_General.lib")

namespace PVX {
	namespace Javascript {
		int Engine::InstanceCount = 0;

		struct V8PrivateData {
			V8PrivateData(v8::Isolate* isolate);
			v8::Isolate::Scope isolate_scope;
			v8::HandleScope handle_scope;
			v8::Local<v8::Context> context;
			v8::Context::Scope context_scope;
			v8Value Global;
			v8::Local<v8::Object> v8Global;
			v8::Isolate * Isolate;
		};
		V8PrivateData::V8PrivateData(v8::Isolate* isolate) :
			Isolate(isolate),
			isolate_scope(isolate),
			handle_scope(isolate),
			context(v8::Context::New(isolate)),
			context_scope(context){
			v8Global = context->Global();
			Global = v8Global;
		}
		void Engine::Reset() {
			Release();
			Init();
		}

#ifdef _DEBUG
		std::string Engine::V8Path = "v8Build\\Debug\\";
#else
		std::string Engine::V8Path = "v8Build\\Release\\";
#endif

		//std::string Engine::V8Path = "";
		void Engine::SetV8Path(const std::string& path) {
			V8Path = path + (path.size() && path.back()!='\\'?"\\":"");
		}
		static std::unique_ptr<v8::Platform> platform;
		void Engine::Release() {
			auto& pd = *((V8PrivateData*)PrivateData);
			auto iso = pd.Isolate;
			delete ((V8PrivateData*)PrivateData);
			iso->Dispose();
		}
		void Engine::Init() {
			v8::Isolate::CreateParams cp;
			cp.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
			PrivateData = new V8PrivateData(v8::Isolate::New(cp));
		}
		Engine::Engine() {
			if (!InstanceCount++) {
				auto v8Path = PVX::IO::FilePathPart(V8Path + "v8.dll");

				LoadLibrary((v8Path + "\\icuuc.dll").c_str());
				LoadLibrary((v8Path + "\\icui18n.dll").c_str());
				LoadLibrary((v8Path + "\\v8_libbase.dll").c_str());
				LoadLibrary((v8Path + "\\v8_libplatform.dll").c_str());
				LoadLibrary((v8Path + "\\v8.dll").c_str());

				v8::V8::InitializeICUDefaultLocation("");
				v8::V8::InitializeExternalStartupData((v8Path + "\\Data").c_str());
				platform = v8::platform::NewDefaultPlatform();
				v8::V8::InitializePlatform(platform.get());
				v8::V8::Initialize();
			}
			Init();
		}
		Engine::~Engine() {
			Release();
			if (!--InstanceCount) {
				v8::V8::Dispose();
				v8::V8::ShutdownPlatform();
			}
		}

		v8::Local<v8::String> Engine::ToString(const char * Text) {
			return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), Text);
		}

		v8Value Engine::RunCode(const std::wstring & Code) {
			return RunCode(ToString(Code));
		}

		v8Value Engine::RunCode(const std::string & Code) {
			return RunCode(ToString(Code.c_str()));
		}
		v8Value Engine::LoadCode(const std::wstring & Filename) {
			return RunCode(ToString(PVX::IO::ReadText(Filename.c_str()).c_str()));
		}
		v8Value Engine::RunCode(const v8::Local<v8::String>& Code) {
			V8PrivateData& pd = *(V8PrivateData*)PrivateData;
			if (auto prog = v8::Script::Compile(pd.context, Code); !prog.IsEmpty())
				if (auto res = prog.ToLocalChecked()->Run(pd.context); !res.IsEmpty())
					return res.ToLocalChecked();
			return v8Value::Type::Undefined;
		}

		PVX::JSON::Item Engine::CallFunction(v8Value & Object, const std::wstring & Function, const std::vector<PVX::JSON::Item> & Params) {
			auto params = PVX::Map(Params, v8Value::FromJson);
			V8PrivateData & pd = *(V8PrivateData*)PrivateData;
			auto fnc = Object[Function].GetValue().As<v8::Function>();
			return ToJson(fnc->Call(pd.context, Object.GetValue(), (int)params.size(), params.size() ? params.data() : nullptr).ToLocalChecked());
		}

		v8Value Engine::CallFunctionV8(v8Value Object, const std::wstring & Function, const std::vector<v8Value>& Params) {
			auto params = PVX::Map(Params, [](const v8Value & v) { return v.GetValue(); });
			V8PrivateData & pd = *(V8PrivateData*)PrivateData;
			if (Object.Contains(Function)) {
				auto fncVal = Object[Function].GetValue();
				auto fnc = fncVal.As<v8::Function>();
				return fnc->Call(pd.context, Object.GetValue(), (int)params.size(), params.size() ? params.data() : nullptr).ToLocalChecked();
			}
			return v8Value::Type::Undefined;
		}
		PVX::JSON::Item Engine::CallFunction(const std::wstring & Function, const std::vector<PVX::JSON::Item> & Params) {
			V8PrivateData & pd = *(V8PrivateData*)PrivateData;
			auto params = PVX::Map(Params, v8Value::FromJson);
			auto name = ToString(Function);
			if (pd.v8Global->Has(pd.context, name).ToChecked()) {
				auto fnc = pd.v8Global->Get(name).As<v8::Function>();
				return ToJson(fnc->Call(pd.context, pd.v8Global, (int)params.size(), params.size() ? params.data() : nullptr).ToLocalChecked());
			}
			return PVX::JSON::jsElementType::Undefined;
		}
		v8Value Engine::CallFunctionV8(const std::wstring & Function, const std::vector<v8Value>& Params) {
			V8PrivateData & pd = *(V8PrivateData*)PrivateData;
			auto params = PVX::Map(Params, [](const v8Value & v) { return v.GetValue(); });
			auto name = ToString(Function);
			if (pd.v8Global->Has(pd.context, name).ToChecked()) {
				auto fnc = pd.v8Global->Get(name).As<v8::Function>();
				return fnc->Call(pd.context, pd.v8Global, (int)params.size(), params.size() ? params.data() : nullptr).ToLocalChecked();
			}
			return v8Value::Type::Undefined;
		}
		v8Value Engine::Call(const v8Value & Name, const std::vector<v8Value>& Params) {
			V8PrivateData & pd = *(V8PrivateData*)PrivateData;
			auto params = PVX::Map(Params, [](const v8Value & v) { return v.GetValue(); });

			if (pd.v8Global->Has(pd.context, Name.GetValue()).ToChecked()) {
				auto fnc = pd.v8Global->Get(Name.GetValue()).As<v8::Function>();
				return fnc->Call(pd.context, pd.v8Global, (int)params.size(), params.size() ? params.data() : nullptr).ToLocalChecked();
			}
			return v8Value::Type::Undefined;
		}
		v8Value Engine::Call(v8Value & Object, const v8Value & Name, const std::vector<v8Value>& Params) {
			V8PrivateData & pd = *(V8PrivateData*)PrivateData;
			auto params = PVX::Map(Params, [](const v8Value & v) { return v.GetValue(); });
			if (Object.GetValue().As<v8::Object>()->Has(pd.context, Name.GetValue()).ToChecked()) {
				auto fnc = Object.GetValue().As<v8::Function>()->Get(Name.GetValue()).As<v8::Function>();
				return fnc->Call(pd.context, Object.GetValue(), (int)params.size(), params.size() ? params.data() : nullptr).ToLocalChecked();
			}
			return v8Value::Type::Undefined;
		}
		v8Value& Engine::operator[](const std::wstring & Name) { return (*(V8PrivateData*)PrivateData).Global[Name]; }

		v8Value& Engine::operator[](const std::string& Name) { return (*(V8PrivateData*)PrivateData).Global[Name]; }

		std::wstring Engine::stringify(const v8Value & obj, const std::wstring& format) {
			if(format.size())
				return PVX::ToString(v8::JSON::Stringify(((V8PrivateData*)PrivateData)->context, obj.GetValue(), ToString(format)).ToLocalChecked());
			return PVX::ToString(v8::JSON::Stringify(((V8PrivateData*)PrivateData)->context, obj.GetValue()).ToLocalChecked());
		}
		std::vector<unsigned char> Engine::stringifyUTF(const v8Value & obj, const std::wstring& format) {
			if (format.size()) {
				auto tmp = v8::String::Utf8Value(((V8PrivateData*)PrivateData)->Isolate, v8::JSON::Stringify(((V8PrivateData*)PrivateData)->context, obj.GetValue(), ToString(format)).ToLocalChecked());
				std::vector<unsigned char> ret(tmp.length());
				memcpy(&ret[0], *tmp, ret.size());
				return ret;
			} else {
				auto tmp = v8::String::Utf8Value(((V8PrivateData*)PrivateData)->Isolate, v8::JSON::Stringify(((V8PrivateData*)PrivateData)->context, obj.GetValue()).ToLocalChecked());
				std::vector<unsigned char> ret(tmp.length());
				memcpy(&ret[0], *tmp, ret.size());
				return ret;
			}
		}
		v8Value Engine::parse(const std::wstring & str) {
			return v8::JSON::Parse(((V8PrivateData*)PrivateData)->context, ToString(str)).ToLocalChecked();
		}
		std::wstring Engine::ToString(const v8::Local<v8::String> & str) {
			std::string x = *v8::String::Utf8Value(((V8PrivateData*)PrivateData)->Isolate, str);
			return PVX::Decode::UTF((unsigned char*)x.data(), (int)(x.size()));
		}
		v8::Local<v8::String> Engine::ToString(const std::wstring & str) {
			return v8::String::NewFromUtf8(((V8PrivateData*)PrivateData)->Isolate, (char*)PVX::Encode::UTF0(str).data());
		}
		v8::Local<v8::String> Engine::ToString(const std::vector<unsigned char>& str) {
			return v8::String::NewFromUtf8(((V8PrivateData*)PrivateData)->Isolate, (char*)str.data());
		}
	}
	std::wstring ToString(const v8::Handle<v8::Value>& val) {
		std::string x = *v8::String::Utf8Value(v8::Isolate::GetCurrent(), val);
		return PVX::Decode::UTF((unsigned char*)x.data(), (int)(x.size()));
	}
	std::wstring ToString(const v8::Handle<v8::String>& val) {
		std::string x = *v8::String::Utf8Value(v8::Isolate::GetCurrent(), val);
		return PVX::Decode::UTF((unsigned char*)x.data(), (int)(x.size()));
	}
	std::vector<unsigned char> ToUtfString(const v8::Handle<v8::String>& val) {
		auto tmp = v8::String::Utf8Value(v8::Isolate::GetCurrent(), val);
		std::vector<unsigned char> ret(tmp.length());
		memcpy(&ret[0], *tmp, ret.size());
		return ret;
	}
	PVX::JSON::Item ToJson(const v8::Handle<v8::Value>& val) {
		if (val->IsInt32()) {
			return val.As<v8::Int32>()->Value();
		} else if(val->IsBoolean()) {
			return val.As<v8::Boolean>()->Value();
		} else if (val->IsNumber()) {
			return val.As<v8::Number>()->Value();
		} else if (val->IsString()) {
			return ToString(val);
		} else if (val->IsArray()) {
			auto Array = val.As<v8::Array>();
			uint32_t len = Array->Length();
			PVX::JSON::Item ret = PVX::JSON::jsElementType::Array;
			for (uint32_t i = 0; i < len; i++) ret.push(ToJson(Array->Get(i)));
			return ret;
		} else if (val->IsObject()) {
			auto obj = val.As<v8::Object>();
			auto Array = obj->GetPropertyNames(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked();
			int len = Array->Length();
			PVX::JSON::Item ret = PVX::JSON::jsElementType::Object;
			for (int i = 0; i < len; i++) {
				auto name = Array->Get(i);
				auto x = ToJson(obj->Get(name));
				if (x.Type() != JSON::jsElementType::Undefined) ret[PVX::ToString(name)] = x;
			}
			return ret;
		}  else if (val->IsNull()) {
			return PVX::JSON::jsElementType::Null;
		}
		return PVX::JSON::jsElementType::Undefined;
	}
}