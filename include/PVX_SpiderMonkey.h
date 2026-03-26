#pragma once
#include<string>
#include<vector>
#include<variant>
#include<memory>
#include<optional>
#include<functional>
#include<filesystem>


struct ContextNode;
namespace PVX::Javascript {
	void Init(int ContextCount = 1);

	class jValue;
	struct jValueData;
	struct jsBool {
		bool val;
		jsBool(bool b): val(b) {}
	};
	enum class jsUndefined { Undefined };

	class jFunc {
	public:
		jFunc(jFunc&&) = default;
		jFunc& operator=(jFunc&&) = default;
		jFunc(const jFunc&) = delete;
		jFunc& operator=(const jFunc&) = delete;
	private:
		const ContextNode* ctx;
		int id;
		std::shared_ptr<jValueData> Data;
		jFunc(const ContextNode* c, int id, const std::shared_ptr<jValueData>& d)
			: ctx(c), id(id), Data(d) {}
		friend class Engine;
		friend class jValue;
		friend jFunc MakeJFunc(ContextNode*, int);
		friend struct JSHelper;
	};

	struct jValueProxy {
		struct jArray {
			std::vector<jValueProxy> vec;
			inline jArray(const std::initializer_list<jValueProxy>& v) {
				for (auto &x : v) vec.emplace_back(std::move(x));
			}
		};
		jValueProxy() = default;
		jValueProxy(const jValueProxy&) = default;
		jValueProxy(jValueProxy&&) = default;
		using vTypes = std::variant<nullptr_t, jsUndefined, jsBool, int64_t, double, std::wstring>;
		enum class jType { Null, Undefined, Bool, Integer, Float, String, Object, Array } Type;
		vTypes value;
		std::unordered_map<std::string, jValueProxy> obj;
		std::vector<jValueProxy> arr;

		inline jValueProxy(const std::initializer_list<std::pair<const char*, jValueProxy>>& t): Type{ jType::Object } {
			for (auto& [k, v] : t) {
				obj.emplace(std::string(k), v);
			}
		}

		inline jValueProxy(const jArray& t): Type{ jType::Array } {
			for (auto&v : t.vec)
				arr.emplace_back(std::move(v));
		}
		inline jValueProxy(nullptr_t) : value{ nullptr }, Type{ jType::Null } {};
		inline jValueProxy(jsUndefined): value{ jsUndefined::Undefined }, Type{ jType::Undefined } {};
		inline jValueProxy(bool v): value{ jsBool(v) }, Type{ jType::Bool } {};
		inline jValueProxy(int64_t v): value{ v }, Type{ jType::Integer } {};
		inline jValueProxy(int v): value{ v }, Type{ jType::Integer } {};
		inline jValueProxy(double v): value{ v }, Type{ jType::Float } {};
		inline jValueProxy(const wchar_t* v): value{ std::wstring(v) }, Type{ jType::String } {};
		inline jValueProxy(const std::wstring& v): value{ std::wstring(v) }, Type{ jType::String } {};
	};

	struct jArg {
		using jArgVar = std::variant<nullptr_t, jsUndefined, jsBool, int64_t, double, std::wstring, jValue*, jFunc*, jValueProxy>;
		jArgVar Value;
		inline jArg(nullptr_t) { Value = nullptr; };
		inline jArg(jsUndefined) { Value = jsUndefined::Undefined; };
		inline jArg(bool v) { Value = jsBool(v); };
		inline jArg(int64_t v) { Value = v; };
		inline jArg(int v) { Value = (int64_t)v; };
		inline jArg(double v) { Value = v; };
		inline jArg(const std::wstring& v) { Value = v; };
		inline jArg(const wchar_t* v): Value{ std::wstring(v) } {};
		inline jArg(jValue* && v): Value{ v } {};
		inline jArg(jValue && v): Value{ &v } {};
		inline jArg(jFunc && v): Value{ &v } {};
		inline jArg(const jValueProxy& v): Value { std::move(v) } {};
		//inline jArg2(const jFunc* v): Value{ *v } {};
		operator jArgVar() { return Value; };
		jArgVar& operator()() { return Value; };
	};
	
	//using jArg = std::variant<nullptr_t, jsUndefined, jsBool, int64_t, double, std::wstring, jValue*, jFunc>;
	//using jArg = jArg2;

	

	class jValue {
	public:
		int64_t Integer();
		double Float();
		std::wstring String();
		jValue operator[](const std::string& name);
		jValue operator[](int index);
		jValue freeze();
		jValue(const jValue&);
		jValue& operator=(const jFunc* value);
		jValue& operator=(const jArg& value);
		jValue& operator=(const jValue& value);
		template<typename... Args>
		jValue operator()(Args&&... args) {
			return callImpl({ jArg(std::forward<Args>(args))... });
		}

		bool isNull() const;
		bool isUndefined() const;
		bool isEmpty() const;
		bool isBool() const;
		bool isInteger() const;
		bool isFloat() const;
		bool isNumber() const;
		bool isString() const;
		bool isObject() const;
		bool isArray() const;
		bool isFunction() const;
		bool operator!() const;

		std::vector<std::string> keys() const;
		void forEach(std::function<void(jValue)> fn);
		void forEach(std::function<void(int, jValue)> fn);
		void forEach(std::function<void(const std::string&, jValue)> fn);

	private:
		const ContextNode* ctx;
		std::shared_ptr<jValueData> Data;
		std::vector<std::variant<int, std::string>> path;
		jValue(const ContextNode* c, const std::shared_ptr<jValueData>& j);
		jValue(const ContextNode* c, const std::shared_ptr<jValueData>& j, const std::vector<std::variant<int, std::string>>& p);
		jValue New(const jArg& arg = nullptr) const;
		jValue callImpl(std::vector<jArg> args);
		friend class Engine;
		friend struct JSHelper;
	};

	class Engine {
		ContextNode * Context;
		std::optional<jValue> Global;
	public:
		Engine();
		~Engine();

		jValue operator[](const std::string& name);
		jValue New(const jArg& arg = nullptr);

		jValue Run(const std::wstring& source);
		jValue RunFile(const std::filesystem::path& source);

		jFunc Function(std::function<jValue(PVX::Javascript::Engine&, std::vector<jValue>)> fn);
		jFunc Function(std::function<jValue(PVX::Javascript::Engine&, jValue, std::vector<jValue>)> fn);
	};
	void Init(std::function<void(Engine&)> init, int ContextCount = 1);
}