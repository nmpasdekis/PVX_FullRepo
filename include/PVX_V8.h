#ifndef __PVX_V8_H__
#define __PVX_V8_H__

#include <../External/V8/include/v8.h>
#include <vector>
#include <map>
#include <functional>
#include <PVX_json.h>
//#include <PVX_Network.h>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <type_traits>
#include <PVX.inl>

namespace PVX {
	std::wstring ToString(const v8::Handle<v8::Value> & val);
	std::wstring ToString(const v8::Handle<v8::String>& val);
	std::vector<unsigned char> ToUtfString(const v8::Handle<v8::String>& val);
	PVX::JSON::Item ToJson(const v8::Handle<v8::Value>& val);

	namespace Javascript {
		class v8Value;

		class Engine {
		public:
			~Engine();
			Engine();

			v8Value RunCode(const std::wstring & Code);
			v8Value RunCode(const std::string & Code);
			v8Value LoadCode(const std::wstring & Filename);
			PVX::JSON::Item CallFunction(const std::wstring & Function, const std::vector<PVX::JSON::Item> & Params = {});
			v8Value CallFunctionV8(const std::wstring & Function, const std::vector<v8Value> & Params = {});
			v8Value CallFunctionV8(v8Value Object, const std::wstring & Function, const std::vector<v8Value> & Params = {});
			v8Value Call(const v8Value & FunctionName, const std::vector<v8Value> & Params = {});
			v8Value Call(v8Value & Object, const v8Value & Function, const std::vector<v8Value> & Params = {});
			PVX::JSON::Item CallFunction(v8Value & Object, const std::wstring & Function, const std::vector<PVX::JSON::Item> & Params = {});

			std::wstring stringify(const v8Value & obj, const std::wstring& format = L"");
			std::vector<unsigned char> stringifyUTF(const v8Value & obj, const std::wstring& format = L"");
			v8Value parse(const std::wstring & str);
			std::wstring ToString(const v8::Local<v8::String> & str);
			v8::Local<v8::String> ToString(const std::vector<unsigned char>& str);
			v8::Local<v8::String> ToString(const std::wstring & str);
			v8::Local<v8::String> ToString(const char * str);

			v8Value& operator[](const std::wstring & Name);
			v8Value& operator[](const std::string& Name);
			void Reset();
			static void SetV8Path(const std::string& path);
		private:
			v8Value RunCode(const v8::Local<v8::String>& Code);
			static std::string V8Path;
			static int InstanceCount;
			void * PrivateData;
			void Release();
			void Init();
		};

		class v8Value {
		public:
			enum class Type {
				Undefined,
				Null,
				Number,
				String,
				Array,
				Object,
				Boolean,
			};
			class Array {
			public:
				Array(const std::vector<v8Value> & arr);
			protected:
				std::vector<v8Value> Arr;
				friend class v8Value;
			};

			~v8Value();
			v8Value();
			v8Value(const Type& v);
			v8Value(const v8::Handle<v8::Value> & v);
			v8Value(const Array & a);
			v8Value(const nullptr_t& v);
			v8Value(const bool& v);
			v8Value(const double& v);
			v8Value(const int& v);
			v8Value(const std::wstring& v);
			v8Value(const std::string& v);
			v8Value(const wchar_t * v);

			v8Value(std::function<PVX::JSON::Item(const std::vector<PVX::JSON::Item>&)> clb);
			v8Value(std::function<v8Value(std::vector<v8Value>&)> clb);
			v8Value(std::function<void(const v8::FunctionCallbackInfo<v8::Value>&)> clb);
			v8Value(std::function<v8Value()> clb);

			inline v8Value(const std::initializer_list<std::pair<std::wstring, v8Value>>& v) : v8Data{ v8::Object::New(v8::Isolate::GetCurrent()) } {
				const auto& obj = v8Data.As<v8::Object>();
				for (auto& [Name, Value] : v)
					(*this)[Name] = Value;
			}
			template<typename T>
			inline v8Value(const std::vector<T>& v) : v8Data{ v8::Array::New(v8::Isolate::GetCurrent()) } {
				const auto& obj = v8Data.As<v8::Array>();
				int i = 0;
				for (auto& vv : v) {
					(*this)[i++] = vv;
				}
			}

			template<typename T>
			inline v8Value(const std::map<std::wstring, T>& v) : v8Data{ v8::Object::New(v8::Isolate::GetCurrent()) } {
				const auto & obj = v8Data.As<v8::Object>();
				for (auto& [Name, Value] : v) {
					(*this)[Name] = Value;
				}
			}

			v8Value& operator=(const v8::Local<v8::Value> & v);
			v8Value& operator=(const v8Value & v);
			v8Value& operator=(const nullptr_t& v);
			v8Value& operator=(const bool& v);
			v8Value& operator=(const double& v);
			v8Value& operator=(const int& v);
			v8Value& operator=(const std::wstring& v);
			v8Value& operator=(const wchar_t * v);

			bool operator!();
			v8Value& operator&&(v8Value& v);
			v8Value& operator||(v8Value& v);

			v8Value& operator=(std::function<PVX::JSON::Item(const std::vector<PVX::JSON::Item>&)> clb);
			v8Value& operator=(std::function<v8Value(std::vector<v8Value>&)> clb);
			v8Value& operator=(std::function<void(const v8::FunctionCallbackInfo<v8::Value>&)> clb);
			v8Value& operator=(std::function<v8Value()> clb);

			template<typename T>
			inline v8Value& operator=(const std::vector<T>& v) {
				return (*this) = v8Value{ v };
			}

			template<typename T>
			inline v8Value& operator=(const std::map<std::wstring, T>& v) {
				return (*this) = v8Value{ v };
			}

			template<typename ...Params>
			v8Value operator()(Params&& ... param) {
				std::vector<v8::Local<v8::Value>> par;
				par.reserve(sizeof...(param));
				(par.push_back(v8Value(param).GetValue()), ...);
				return v8Data.As<v8::Function>()->Call(
					v8::Isolate::GetCurrent()->GetCurrentContext(),
					Parent,
					(int)par.size(),
					par.size() ? par.data() : nullptr).ToLocalChecked();
			}
			//template<>
			//v8Value operator()() {
			//	return v8Data.As<v8::Function>()->Call(
			//		v8::Isolate::GetCurrent()->GetCurrentContext(),
			//		Parent,
			//		0,
			//		nullptr).ToLocalChecked();
			//}

			std::vector<std::wstring> keys();
			int Length();

			v8::Local<v8::Value> GetValue() const;

			Type GetType();

			bool Contains(const std::wstring & Member);

			void forEach(std::function<void(const std::wstring&, const v8Value& Item)> fnc);
			void forEach(std::function<void(int, const v8Value& Item)> fnc);
			void forEach(std::function<void(const v8Value& Item)> fnc);
			v8Value map(std::function<v8Value(const v8Value&)> fnc);
			v8Value filter(std::function<bool(const v8Value&)> fnc);

			bool IsNull() const;
			bool IsUndefined() const;
			bool IsNullOrUndefined() const;
			bool IsZero() const;
			bool IsArray() const;
			bool IsObject() const;
			bool IsEmpty() const;
			bool Boolean() const;
			double Double() const;
			int Integer() const;
			v8Value Call(const std::vector<v8Value> & args);
			v8Value Call(v8Value & This, const std::vector<v8Value> & args);
			std::wstring String() const;
			v8Value& operator[](int Index);
			v8Value& operator[](const std::wstring & Index);
			v8Value& operator[](const std::string & Index);
			bool Has(const std::wstring & Name);

			PVX::JSON::Item ToJson();

			v8Value Reduce();

			static v8::Local<v8::Value> FromJson(const PVX::JSON::Item & Val);

			int GetId() { return Id; }
		protected:
			std::map<int, v8Value> ArrayChild;
			std::map<std::wstring, v8Value> ObjectChild;
			v8Value& SetValue(const v8Value & v);
			v8Value& SetValue(const v8::Local<v8::Value> & v);
			v8Value(const v8::Local<v8::Value> & v, const v8::Local<v8::Value> & Parent, const std::wstring& Index);
			v8Value(const v8::Local<v8::Value> & v, const v8::Local<v8::Value> & Parent, const v8::Local<v8::String>& Index);
			v8Value(const v8::Local<v8::Value> & v, const v8::Local<v8::Value> & Parent, const int& Index);

			v8Value& Function(std::function<PVX::JSON::Item(const std::vector<PVX::JSON::Item>&)> clb);
			v8Value& Function(std::function<v8Value(std::vector<v8Value>&)> clb);
			v8Value& Function(std::function<void(const v8::FunctionCallbackInfo<v8::Value>&)> clb);

			v8::Local<v8::Value> v8Data, Parent;
			v8::Local<v8::String> StringIndex;
			int Index = 0;

			PVX::RefCounter Ref;
			static int NextId;
			int Id = [this] { return NextId++; }();
		};

		class AsyncEngine {
			std::thread Looper;
			std::condition_variable MainThreadCV, ThreadCV;
			std::mutex taskMod;
			std::atomic_bool Running;
			std::function<void(Engine& eng)> Task = nullptr;
		public:
			AsyncEngine();
			~AsyncEngine();

			PVX::JSON::Item Do(std::function<JSON::Item(Engine& eng)> Task);
			void Do_void(std::function<void(Engine& eng)> Task);

			PVX::JSON::Item RunCode(const std::wstring& Code);
			void RunCode_void(const std::wstring& Code);

			PVX::JSON::Item RunFile(const std::wstring& Code);
			void RunFile_void(const std::wstring& Code);

			PVX::JSON::Item Call(const std::wstring& Function, const std::vector<PVX::JSON::Item>& Params = {});
			void Call_void(const std::wstring& Function, const std::vector<PVX::JSON::Item>& Params = {});
		};
		void Defaults(PVX::Javascript::Engine& Engine);
	}
}

#endif