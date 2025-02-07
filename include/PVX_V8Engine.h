#pragma once
#include <v8.h>
#include <string>
#include <memory>
#include <PVX_json.h>

namespace PVX::Javascript {

	template<typename T>
	auto get(const v8::Local<T>& v) {
		return v->Value();
	}
	template<typename T>
	T get(const v8::MaybeLocal<T>& v) {
		T ret{};
		v.To(&ret);
		return ret;
	}
	template<typename T>
	T get(const v8::Maybe<T>& v) {
		T ret{};
		v.To(&ret);
		return ret;
	}

	inline int getInt(const v8::Local<v8::Value> &v) {
		return get<int>(v->Int32Value(v8::Isolate::GetCurrent()->GetCurrentContext()));
	}

	v8::Local<v8::String> ToString(const char* Text);
	v8::Local<v8::String> ToString(const wchar_t* Text);
	//std::wstring ToString(const v8::Handle<v8::String>& str);
	std::wstring ToString(const v8::Handle<v8::Value>& str);
	PVX::JSON::Item GetJson(const v8::Local<v8::Value>& val, v8::Isolate* Iso = v8::Isolate::GetCurrent(), v8::Local<v8::Context> ctx = v8::Isolate::GetCurrent()->GetCurrentContext());
	v8::Local<v8::Value> FromJson(const PVX::JSON::Item& val);

	class Engine {
	public:
		Engine();

		v8::Local<v8::Object>& Global();

		v8::Local<v8::Value> Run(std::string_view Code);
		v8::Local<v8::Value> Run(std::wstring_view Code);
		PVX::JSON::Item RunJson(std::string_view Code);
		PVX::JSON::Item RunJson(std::wstring_view Code);

		v8::Local<v8::Value> Make(std::function<JSON::Item(const std::vector<JSON::Item>& Params)> fnc);
		v8::Local<v8::Value> Make(std::function<void(const v8::FunctionCallbackInfo<v8::Value>& args)> fnc);

		PVX::JSON::Item Get(const v8::Local<v8::Value>& val);
		std::function<PVX::JSON::Item(const std::initializer_list<PVX::JSON::Item>& params)> GetFunction(const std::wstring& Name);
		void Set(v8::Local<v8::Value>& Object, std::wstring Name, const v8::Local<v8::Value>& Value);
		void Set(v8::Local<v8::Object>& Object, std::wstring Name, const v8::Local<v8::Value>& Value);
		void Set(std::wstring Name, const v8::Local<v8::Value>& Value);
		void Set(std::string Name, const v8::Local<v8::Value>& Value);
		void Set(std::string Name, const v8::Local<v8::Object>& Value);

		std::function<PVX::JSON::Item(const std::initializer_list<PVX::JSON::Item>& params)> ToFunction(const v8::Handle<v8::Value>& Val);

		inline void Idle() {
			while(!Isolate->IdleNotificationDeadline(0.0f));
			Isolate->LowMemoryNotification();
		}
	private:
		v8::Isolate* Isolate;
		v8::Isolate::Scope isolate_scope;
		v8::HandleScope handle_scope;
		v8::Local<v8::Context> context;
		v8::Context::Scope context_scope;
		v8::Local<v8::Object> v8Global;
	};
}