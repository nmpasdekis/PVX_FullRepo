#include<windows.h>

#include<vector>
#include<mutex>
#include<condition_variable>
#include<PVX_SpiderMonkey.h>
#include<PVX_File.h>
//#define MOZILLA_INTERNAL_API
#define XP_WIN
#include <jsapi.h>
#include <js/Initialization.h>
#include <js/Context.h>
#include <js/RealmOptions.h>
#include <js/GlobalObject.h>
#include <js/Realm.h>
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>
#include <js/experimental/JSStencil.h>
#include <PVX_Encode.h>
#include <js/array.h>
#include <js/Object.h>
#include <js/PropertyAndElement.h>
#include <jsfriendapi.h>
#include <js/JSON.h>

using NativeFunction = std::variant<
	std::function<PVX::Javascript::jValue(PVX::Javascript::Engine&, std::vector<PVX::Javascript::jValue>)>,
	std::function<PVX::Javascript::jValue(PVX::Javascript::Engine&, PVX::Javascript::jValue, std::vector<PVX::Javascript::jValue>)>,
	std::function<void(PVX::Javascript::Engine&, std::vector<PVX::Javascript::jValue>)>,
	std::function<void(PVX::Javascript::Engine&, PVX::Javascript::jValue, std::vector<PVX::Javascript::jValue>)>

> ;

struct ContextNode {
	std::unique_ptr<JSContext, decltype(&JS_DestroyContext)> ctx{ JS_NewContext(JS::DefaultHeapMaxBytes), JS_DestroyContext };
	//JS::PersistentRooted<JSObject*> global;
	std::optional<JS::PersistentRooted<JSObject*>> global;
	JSAutoRealm* realm = nullptr;
	ContextNode* next = nullptr;
	PVX::Javascript::Engine* Engine;
	std::vector<NativeFunction> Functions;
	RefPtr<JS::Stencil> stencil = nullptr;
	ContextNode() {
		JS::InitSelfHostedCode(ctx.get());
	}
};

namespace {
	struct SpiderMonkeyInit {
		SpiderMonkeyInit() {
			JS_Init();
		}
		~SpiderMonkeyInit() {
			JS_ShutDown();
		}
	} SpiderMonkeyInit_;


	static JSClass globalClass = {
		"global",
		JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_RESERVED_SLOTS(1),
		&JS::DefaultGlobalClassOps
	};

	//std::mutex ContextAcquisitionMutex;
	//std::condition_variable WaitForContext;
	//std::vector<ContextNode> Contexts;
	//ContextNode* NextContext;

	//ContextNode * Acquire() {
	//	std::unique_lock<std::mutex> lock{ ContextAcquisitionMutex };
	//	WaitForContext.wait(lock, [] { return NextContext != nullptr; });
	//	auto ret = NextContext;
	//	NextContext = NextContext->next;
	//	return ret;
	//}

	//void Release(ContextNode* ctx) {
	//	{
	//		std::lock_guard<std::mutex> lock{ ContextAcquisitionMutex };
	//		ctx->next = NextContext;
	//		NextContext = ctx;
	//	}
	//	WaitForContext.notify_one();
	//}

	ContextNode * Acquire() {
		thread_local ContextNode ctx;
		return &ctx;
	}
	void Release(ContextNode* ctx) {}
}



namespace PVX::Javascript {
	struct jValueData {
		JS::PersistentRooted<JS::Value> val;
		jValueData(const JS::PersistentRooted<JS::Value>& cx): val(cx) {}
		jValueData(JSContext* cx): val(cx) {}
	};

	std::wstring StencilCode;
	void SetStencilCode(const std::wstring& code) {
		StencilCode = code;
	}
	struct JSHelper {
		static jValue makeJValue(const ContextNode* c, const std::shared_ptr<jValueData>& d) {
			return jValue(c, d);
		}
		static jValueData* getData(const jValue& v) {
			return v.Data.get();
		}
		static jFunc makeJFunc(ContextNode* node, int id, const std::shared_ptr<jValueData>& d) {
			return jFunc(node, id, d);
		}
		static jValue getException(const ContextNode* c) {
			auto data = std::make_shared<jValueData>(c->ctx.get());
			if (JS_IsExceptionPending(c->ctx.get())) {
				if (JS_GetPendingException(c->ctx.get(), &data->val)) {
					JS_ClearPendingException(c->ctx.get());
				}
			}
			return jValue(c, data);
		}
		static std::runtime_error jsException(const ContextNode* c, const char * defMessage) {
			auto data = std::make_shared<jValueData>(c->ctx.get());
			auto err = jValue(c, data);
			if (JS_IsExceptionPending(c->ctx.get())) {
				if (JS_GetPendingException(c->ctx.get(), &data->val)) {
					JS_ClearPendingException(c->ctx.get());
					auto msg = err["message"].String();
					auto stack = err["stack"].String();
					return std::runtime_error(PVX::Encode::ToString(msg + L"\n" + stack));
				}
			}
			return std::runtime_error(defMessage);
		}
		static JS::PersistentRooted<JS::Value> fromProxy(const ContextNode* c, const jValueProxy& v) {
			JSContext* cx = c->ctx.get();
			JS::PersistentRooted<JS::Value> val(cx);
			switch (v.Type) {
				case jValueProxy::jType::Null: val.set(JS::NullValue()); break;
				case jValueProxy::jType::Undefined: val.set(JS::UndefinedValue()); break;
				case jValueProxy::jType::Bool: val.set(JS::BooleanValue(std::get<jsBool>(v.value).val)); break;
				case jValueProxy::jType::Integer: val.set(JS::NumberValue(static_cast<double>(std::get<int64_t>(v.value)))); break;
				case jValueProxy::jType::Float: val.set(JS::NumberValue(std::get<double>(v.value))); break;
				case jValueProxy::jType::String: {
					JS::RootedString str(cx, JS_NewUCStringCopyN(cx, reinterpret_cast<const char16_t*>(std::get<std::wstring>(v.value).c_str()), std::get<std::wstring>(v.value).size()));
					val.set(JS::StringValue(str));
				} break;
				case jValueProxy::jType::Object: {
					JS::RootedObject obj(cx, JS_NewPlainObject(cx));
					val.set(JS::ObjectValue(*obj));
					for (auto& [k, v] : v.obj) //ret[k] = fromProxy(c, v);
						JS_SetProperty(cx, obj, k.c_str(), fromProxy(c, v));
				} break;
				case jValueProxy::jType::Array: {
					JS::RootedObject obj(cx, JS::NewArrayObject(cx, v.arr.size()));
					val.set(JS::ObjectValue(*obj));
					int i = 0;
					for (auto& v : v.arr) //ret[i++] = fromProxy(c, v);
						JS_SetElement(cx, obj, i++, fromProxy(c, v));
				} break;
			}
			return val;
		}
		static void ApplyStencil(ContextNode& c) {
			JSContext* cx = c.ctx.get();

			if (!c.stencil) {
				if (StencilCode.empty()) return;

				JS::SourceText<char16_t> source;
				if (!source.init(cx,
					reinterpret_cast<const char16_t*>(StencilCode.c_str()),
					StencilCode.size(),
					JS::SourceOwnership::Borrowed))
					throw std::runtime_error("Stencil source init failed");

				JS::CompileOptions options(cx);
				options.setFileAndLine("stencil", 1);

				c.stencil = JS::CompileGlobalScriptToStencil(cx, options, source);
				if (!c.stencil)
					throw std::runtime_error("Stencil compilation failed");
			}

			JS::InstantiateOptions instantiateOptions;
			JS::RootedScript script(cx,
				JS::InstantiateGlobalStencil(cx, instantiateOptions, c.stencil));
			if (!script)
				throw std::runtime_error("Stencil instantiation failed");

			JS::RootedValue rval(cx);
			if (!JS_ExecuteScript(cx, script, &rval))
				throw std::runtime_error("Stencil execution failed");
		}
	};

	static bool NativeFunctionCallback(JSContext* cx, unsigned int argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		JS::RootedObject callee(cx, &args.callee());
		JS::Value idSlot = JS::GetReservedSlot(callee, 0);
		int id = idSlot.toInt32();

		JS::RootedObject global(cx, JS::CurrentGlobalOrNull(cx));
		ContextNode* node = static_cast<ContextNode*>(JS::GetReservedSlot(global, 0).toPrivate());

		auto fn = node->Functions[id];

		std::vector<jValue> jArgs;
		jArgs.reserve(argc);
		for (unsigned i = 0; i < argc; i++) {
			auto data = std::make_shared<jValueData>(cx);
			data->val.set(args[i]);
			jArgs.emplace_back(JSHelper::makeJValue(node, data));
		}

		try {
			jValue result = std::visit([&](auto&& f) -> jValue {
				using T = std::decay_t<decltype(f)>;
				if constexpr (std::is_same_v<T, std::function<jValue(Engine&, std::vector<jValue>)>>) {
					return f(*node->Engine, jArgs);
				}
				else if constexpr(std::is_same_v<T, std::function<void(Engine&, std::vector<jValue>)>>) {
					f(*node->Engine, jArgs);

					auto data = std::make_shared<jValueData>(cx);
					data->val.setUndefined();
					return JSHelper::makeJValue(node, data);
				}
				else if constexpr(std::is_same_v<T, std::function<void(Engine&, jValue, std::vector<jValue>)>>) {
					auto thisData = std::make_shared<jValueData>(cx);
					thisData->val.set(args.thisv());
					jValue thisVal = JSHelper::makeJValue(node, thisData);
					f(*node->Engine, thisVal, jArgs);

					auto data = std::make_shared<jValueData>(cx);
					data->val.setUndefined();
					return JSHelper::makeJValue(node, data);
				} 
				else {
					auto thisData = std::make_shared<jValueData>(cx);
					thisData->val.set(args.thisv());
					jValue thisVal = JSHelper::makeJValue(node, thisData);
					return f(*node->Engine, thisVal, jArgs);
				}
			}, fn);

			args.rval().set(JSHelper::getData(result)->val.get());
			return true;
		} catch (std::exception& e) {
			JS_ReportErrorASCII(cx, e.what());
			return false;
		}
	}

	static jFunc MakeJFunc(ContextNode* node, int id) {
		JSContext* cx = node->ctx.get();
		JS::RootedFunction jsFunc(cx,
			JS_NewFunction(cx, NativeFunctionCallback, 0, 0, nullptr));
		JS::RootedObject fnObj(cx, JS_GetFunctionObject(jsFunc));
		JS::SetReservedSlot(fnObj, 0, JS::Int32Value(id));
		auto data = std::make_shared<jValueData>(cx);
		data->val.set(JS::ObjectValue(*fnObj));
		return jFunc(node, id, data);
	}

	jFunc Engine::Function_with_result(std::function<jValue(Engine&, std::vector<jValue>)> fn) {
		int id = (int)Context->Functions.size();
		Context->Functions.push_back(std::move(fn));
		return MakeJFunc(Context, id);
	}

	jFunc Engine::Function_with_result(std::function<jValue(Engine&, jValue, std::vector<jValue>)> fn) {
		int id = (int)Context->Functions.size();
		Context->Functions.push_back(std::move(fn));
		return MakeJFunc(Context, id);
	}
	jFunc Engine::Function(std::function<void(Engine&, std::vector<jValue>)> fn) {
		int id = (int)Context->Functions.size();
		Context->Functions.push_back(std::move(fn));
		return MakeJFunc(Context, id);
	}

	jFunc Engine::Function(std::function<void(Engine&, jValue, std::vector<jValue>)> fn) {
		int id = (int)Context->Functions.size();
		Context->Functions.push_back(std::move(fn));
		return MakeJFunc(Context, id);
	}

	std::function<void(Engine&)> initFunction = nullptr;
	std::function<void(Engine&)> initFunction2 = nullptr;

	void Init(std::function<void(Engine&)> init) {
		initFunction = init;
	}
	void Init2(std::function<void(Engine&)> init) {
		initFunction2 = init;
	}
	Engine::Engine(): Context{ Acquire() } {
		ContextNode& c = *Context;
		c.Engine = this;
		JS::RealmOptions options;
		c.global.emplace(c.ctx.get(), JS_NewGlobalObject(c.ctx.get(), &globalClass, nullptr,
			JS::FireOnNewGlobalHook, options));
		JS::SetReservedSlot(c.global->get(), 0, JS::PrivateValue(Context));
		c.realm = new JSAutoRealm(c.ctx.get(), c.global.value());
		JS::InitRealmStandardClasses(c.ctx.get());

		auto data = std::make_shared<jValueData>(c.ctx.get());
		data->val.set(JS::ObjectValue(*c.global.value().get()));
		Global.emplace(jValue(Context, data));
		if (initFunction) initFunction(*this);
		JSHelper::ApplyStencil(c);
		if (initFunction2) initFunction2(*this);
	}
	Engine::~Engine() {
		Global.reset();
		ContextNode& c = *Context;
		c.Functions.clear();
		c.global.value().reset();
		c.global.reset();
		delete c.realm;
		c.realm = nullptr;
	}


	jValue Engine::Run(const std::wstring& src) {
		ContextNode& c = *Context;
		auto ctx = c.ctx.get();
		//JS::RootedValue result(ctx);
		auto ret = Global->New();

		//JS::SourceText<mozilla::Utf8Unit> source;
		JS::SourceText<char16_t> source;
		source.init(ctx, (char16_t*)src.c_str(), src.size(), JS::SourceOwnership::Borrowed);

		JS::CompileOptions opts(ctx);
		if(!JS::Evaluate(ctx, opts, source, &(ret.Data->val)))
			throw JSHelper::jsException(Context, "Fail to run");
		return ret;
	}
	jValue Engine::RunFile(const std::filesystem::path& filename) {
		auto src = L"//# sourceURL=" + filename.wstring() + L"\n" + PVX::IO::ReadUtf(filename);
		ContextNode& c = *Context;
		auto ctx = c.ctx.get();
		auto ret = Global->New();

		JS::SourceText<char16_t> source;
		source.init(ctx, (char16_t*)src.c_str(), src.size(), JS::SourceOwnership::Borrowed);

		JS::CompileOptions opts(ctx);
		if(!JS::Evaluate(ctx, opts, source, &(ret.Data->val)))
			throw JSHelper::jsException(Context, "Fail to run");
		return ret;
	}

	namespace {
		JS::Value resolvePath(
			JSContext* cx,
			JS::Value root,
			const std::vector<std::variant<int, std::string>>& path) {
			JS::RootedValue current(cx, root);
			for (auto& step : path) {
				if (!current.isObject())
					throw std::runtime_error("Path step is not an object");
				JS::RootedValue next(cx);
				JS::RootedObject obj(cx, &current.toObject());
				if (std::holds_alternative<std::string>(step)) {
					if (!JS_GetProperty(cx, obj, std::get<std::string>(step).c_str(), &next))
						throw std::runtime_error("Property not found: " + std::get<std::string>(step));
				}
				else {
					if (!JS_GetElement(cx, obj, std::get<int>(step), &next))
						throw std::runtime_error("Index not found");
				}
				current = next;
			}
			return current;
		}

		bool resolvePathForAssignment(JSContext* cx, JS::MutableHandleObject parent, const std::vector<std::variant<int, std::string>>& path, std::variant<int, std::string>& lastKey) {
			JS::RootedObject current(cx, parent.get());
			for (auto i = 0; i + 1 < path.size(); i++) {
				auto& step = path[i];
				auto& nextStep = path[i + 1];
				JS::RootedValue next(cx);

				if (std::holds_alternative<std::string>(step)) {
					if (!JS_GetProperty(cx, current, std::get<std::string>(step).c_str(), &next))
						return false;
				}
				else {
					if (!JS_GetElement(cx, current, std::get<int>(step), &next))
						return false;
				}

				if (next.isUndefined()) {
					JS::RootedObject newObj(cx,
						std::holds_alternative<std::string>(nextStep)
						? JS_NewPlainObject(cx)
						: JS::NewArrayObject(cx, 0));
					if (!newObj) return false;
					JS::RootedValue newVal(cx, JS::ObjectValue(*newObj));
					if (std::holds_alternative<std::string>(step)) {
						if (!JS_SetProperty(cx, current, std::get<std::string>(step).c_str(), newVal))
							return false;
					}
					else {
						if (!JS_SetElement(cx, current, std::get<int>(step), newVal))
							return false;
					}
					current = newObj;
				}
				else if (next.isObject()) {
					current = &next.toObject();
				}
				else {
					return false;
				}
			}
			parent.set(current);
			lastKey = path.back();
			return true;
		}
	}

	jValue Engine::ParseJSON(const std::wstring& json) {
		JSContext* cx = Context->ctx.get();
		JS::RootedValue result(cx);
		if (!JS_ParseJSON(cx,
			reinterpret_cast<const char16_t*>(json.c_str()),
			json.size(),
			&result))
			throw std::runtime_error("JSON parse failed");
		auto data = std::make_shared<jValueData>(cx);
		data->val.set(result);
		return JSHelper::makeJValue(Context, data);
	}

	std::wstring Engine::StringifyJSON(const jValue& value, bool format) {
		JSContext* cx = Context->ctx.get();
		JS::Value resolved = resolvePath(cx, value.Data->val.get(), value.path);
		JS::RootedValue val(cx, resolved);

		std::wstring result;
		auto callback = [](const char16_t* buf, uint32_t len, void* data) -> bool {
			auto* out = static_cast<std::wstring*>(data);
			out->append(reinterpret_cast<const wchar_t*>(buf), len);
			return true;
		};

		JS::RootedValue space(cx);
		if (format) {
			JS::RootedString tab(cx, JS_NewStringCopyZ(cx, "\t"));
			space.setString(tab);
		}
		else {
			space.setUndefined();
		}
		if (!JS_Stringify(cx, &val, nullptr, space, callback, &result))
			throw std::runtime_error("JSON stringify failed");

		return result;
	}


	jValue::jValue(const ContextNode* c, const std::shared_ptr<jValueData>& j)
		: ctx(c), Data(j) {}

	jValue::jValue(const ContextNode* c, const std::shared_ptr<jValueData>& j,
		const std::vector<std::variant<int, std::string>>& p)
		: ctx(c), Data(j), path(p) {}

	jValue::jValue(const jValue& other) {
		ctx = other.ctx;
		JS::Value resolved = resolvePath(ctx->ctx.get(), other.Data->val.get(), other.path);
		Data = std::make_shared<jValueData>(ctx->ctx.get());
		Data->val.set(resolved);
	}

	jValue& jValue::operator=(const jFunc* value) {
		JSContext* cx = ctx->ctx.get();
		if (path.empty())
			throw std::runtime_error("Cannot assign to root value directly");

		JS::RootedObject parent(cx, &Data->val.toObject());
		std::variant<int, std::string> lastKey;
		if (!resolvePathForAssignment(cx, &parent, path, lastKey))
			throw std::runtime_error("Invalid path for assignment");

		JS::RootedValue fnVal(cx, value->Data->val.get());

		if (std::holds_alternative<std::string>(lastKey)) {
			if (!JS_SetProperty(cx, parent, std::get<std::string>(lastKey).c_str(), fnVal))
				throw std::runtime_error("Assignment failed");
		}
		else {
			if (!JS_SetElement(cx, parent, std::get<int>(lastKey), fnVal))
				throw std::runtime_error("Assignment failed");
		}

		Data->val.set(fnVal);
		path.clear();
		return *this;
	}

	jValue jValue::New(const jArg& arg) const {
		JSContext* cx = ctx->ctx.get();
		auto newData = std::make_shared<jValueData>(cx);
		auto ret = jValue(ctx, newData);
		std::visit([&](auto&& v) {
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::is_same_v<T, nullptr_t>) {
				newData->val.set(JS::NullValue());
			}
			else if constexpr (std::is_same_v<T, jsUndefined>) {
				newData->val.set(JS::UndefinedValue());
			}
			else if constexpr (std::is_same_v<T, jsBool>) {
				newData->val.set(JS::BooleanValue(v.val));
			}
			else if constexpr (std::is_same_v<T, int64_t>) {
				newData->val.set(JS::NumberValue(static_cast<double>(v)));
			}
			else if constexpr (std::is_same_v<T, double>) {
				newData->val.set(JS::NumberValue(v));
			}
			else if constexpr (std::is_same_v<T, std::wstring>) {
				JS::RootedString str(cx,
					JS_NewUCStringCopyN(cx,
						reinterpret_cast<const char16_t*>(v.c_str()),
						v.size()));
				newData->val.set(JS::StringValue(str));
			}
			else if constexpr (std::is_same_v<T, jValue*>) {
				JS::Value resolved = resolvePath(cx, v->Data->val.get(), v->path);
				newData->val.set(resolved);
			}
			else if constexpr (std::is_same_v<T, jFunc*>) {
				newData->val.set(v->Data->val.get());
			}
			else if constexpr (std::is_same_v<T, jValueProxy>) {
				newData->val.set(JSHelper::fromProxy(ctx, v));
			}
		}, arg.Value);
		return ret;
	}

	jValue jValue::operator[](const std::string& name) {
		auto newPath = path;
		newPath.push_back(name);
		return jValue(ctx, Data, newPath);
	}

	jValue jValue::operator[](int index) {
		auto newPath = path;
		newPath.push_back(index);
		return jValue(ctx, Data, newPath);
	}

	jValue jValue::freeze() {
		JSContext* cx = ctx->ctx.get();
		JS::Value resolved = resolvePath(cx, Data->val.get(), path);
		auto newData = std::make_shared<jValueData>(cx);
		newData->val.set(resolved);
		return jValue(ctx, newData);
	}

	jValue jValue::callImpl(std::vector<jArg> args) {
		JSContext* cx = ctx->ctx.get();

		if (path.empty())
			throw std::runtime_error("Cannot invoke: no function in path");

		auto parentPath = std::vector<std::variant<int, std::string>>(path.begin(), path.end() - 1);
		auto& lastStep = path.back();

		JS::RootedValue parentVal(cx, resolvePath(cx, Data->val.get(), parentPath));
		if (!parentVal.isObject())
			throw std::runtime_error("Cannot invoke: parent is not an object");
		JS::RootedObject parent(cx, &parentVal.toObject());

		std::vector<JS::Value> argVals;
		argVals.reserve(args.size());
		for (auto& arg : args)
			argVals.push_back(New(arg).Data->val.get());

		JS::HandleValueArray jsArgs = JS::HandleValueArray::fromMarkedLocation(
			argVals.size(), argVals.data());

		JS::RootedValue rval(cx);
		if (std::holds_alternative<std::string>(lastStep)) {
			if (!JS_CallFunctionName(cx, parent, std::get<std::string>(lastStep).c_str(), jsArgs, &rval))
				throw JSHelper::jsException(ctx, "Function call failed");
				//throw std::runtime_error("Function call failed");
		}
		else {
			JS::RootedValue func(cx);
			if (!JS_GetElement(cx, parent, std::get<int>(lastStep), &func))
				throw std::runtime_error("Function not found at index");
			if (!JS_CallFunctionValue(cx, parent, func, jsArgs, &rval))
				throw JSHelper::jsException(ctx, "Function call failed");
				//throw std::runtime_error("Function call failed");
		}

		auto newData = std::make_shared<jValueData>(cx);
		newData->val.set(rval);
		return jValue(ctx, newData);
	}

	jValue& jValue::operator=(const jArg& value) {
		JSContext* cx = ctx->ctx.get();

		jValue converted = New(value);
		JS::RootedValue jsVal(cx, converted.Data->val.get());

		if (path.empty()) {
			Data->val.set(jsVal);
		}
		else{
			JS::RootedObject parent(cx, &Data->val.toObject());
			std::variant<int, std::string> lastKey;
			if (!resolvePathForAssignment(cx, &parent, path, lastKey))
				throw std::runtime_error("Invalid path for assignment");

			if (std::holds_alternative<std::string>(lastKey)) {
				if (!JS_SetProperty(cx, parent, std::get<std::string>(lastKey).c_str(), jsVal))
					throw std::runtime_error("Assignment failed");
			}
			else {
				if (!JS_SetElement(cx, parent, std::get<int>(lastKey), jsVal))
					throw std::runtime_error("Assignment failed");
			}
		}

		return *this;
	}

	jValue& jValue::operator=(const jValue& value) {
		//auto vv = resolvePath(ctx->ctx.get(), value.Data->val.get(), value.path);
		//Data = std::make_shared<jValueData>(ctx->ctx.get());
		//Data->val.set(vv);
		//path.clear();
		//return *this;
		return operator=(jArg(const_cast<jValue*>(&value)));
	}

	int64_t jValue::Integer() {
		JS::Value resolved = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!resolved.isNumber())
			throw std::runtime_error("Value is not a number");
		return static_cast<int64_t>(resolved.toNumber());
	}

	double jValue::Float() {
		JS::Value resolved = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!resolved.isNumber())
			throw std::runtime_error("Value is not a number");
		return resolved.toNumber();
	}

	std::wstring jValue::String() {
		JS::Value resolved = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!resolved.isString())
			throw std::runtime_error("Value is not a string");
		JS::RootedString str(ctx->ctx.get(), resolved.toString());
		JS::UniqueChars chars = JS_EncodeStringToUTF8(ctx->ctx.get(), str);
		std::wstring ret = PVX::Decode::UTF(chars.get(), strlen(chars.get()));
		return ret;
	}

	jValue Engine::operator[](const std::string& name) {
		return Global->operator[](name);
	}

	jValue Engine::New(const jArg& arg) {
		return Global->New(arg);
	}

	bool jValue::isNull() const {
		return resolvePath(ctx->ctx.get(), Data->val.get(), path).isNull();
	}

	bool jValue::isUndefined() const {
		return resolvePath(ctx->ctx.get(), Data->val.get(), path).isUndefined();
	}

	bool jValue::isEmpty() const {
		auto v = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		return v.isNull() || v.isUndefined();
	}

	bool jValue::isBool() const {
		return resolvePath(ctx->ctx.get(), Data->val.get(), path).isBoolean();
	}
	bool jValue::isNumber() const {
		return resolvePath(ctx->ctx.get(), Data->val.get(), path).isNumber();
	}

	bool jValue::isString() const {
		return resolvePath(ctx->ctx.get(), Data->val.get(), path).isString();
	}

	bool jValue::isObject() const {
		auto v = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!v.isObject()) return false;
		JSContext* cx = ctx->ctx.get();
		JS::RootedObject obj(cx, &v.toObject());
		bool result = false;
		JS::IsArrayObject(cx, obj, &result);
		return !result;
	}

	bool jValue::isArray() const {
		auto v = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!v.isObject()) return false;
		JSContext* cx = ctx->ctx.get();
		JS::RootedObject obj(cx, &v.toObject());
		bool result = false;
		JS::IsArrayObject(cx, obj, &result);
		return result;
	}
	
	bool jValue::operator!() const {
		JSContext* cx = ctx->ctx.get();
		auto v = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (v.isNull() || v.isUndefined()) return true;
		if (v.isBoolean()) return !v.toBoolean();
		if (v.isNumber()) return v.toNumber() == 0.0 || std::isnan(v.toNumber());
		if (v.isString()) {
			JS::RootedString str(cx, v.toString());
			return JS_GetStringLength(str) == 0;
		}
		return false; // objects and functions are always truthy
	}

	bool jValue::isFunction() const {
		auto v = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!v.isObject()) return false;
		JSContext* cx = ctx->ctx.get();
		JS::RootedObject obj(cx, &v.toObject());
		return JS_ObjectIsFunction(obj);
	}

	std::vector<std::string> jValue::keys() const {
		JSContext* cx = ctx->ctx.get();
		auto v = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!v.isObject())
			throw std::runtime_error("keys() called on non-object");

		JS::RootedObject obj(cx, &v.toObject());
		JS::RootedIdVector props(cx);
		if (!js::GetPropertyKeys(cx, obj, JSITER_OWNONLY, &props))
			throw std::runtime_error("keys() failed");

		std::vector<std::string> result;
		result.reserve(props.length());
		for (size_t i = 0; i < props.length(); i++) {
			JS::RootedId id(cx, props[i]);
			JS::RootedValue idVal(cx);
			if (!JS_IdToValue(cx, id, &idVal)) continue;
			if (idVal.isString()) {
				JS::RootedString str(cx, idVal.toString());
				JS::UniqueChars chars = JS_EncodeStringToUTF8(cx, str);
				result.emplace_back(chars.get());
			}
		}
		return result;
	}

	void jValue::forEach(std::function<void(jValue)> fn) {
		JSContext* cx = ctx->ctx.get();
		auto v = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!v.isObject())
			throw std::runtime_error("forEach called on non-array");

		JS::RootedObject obj(cx, &v.toObject());
		bool isArray = false;
		JS::IsArrayObject(cx, obj, &isArray);
		if (!isArray)
			throw std::runtime_error("forEach called on non-array");

		uint32_t length = 0;
		if (!JS::GetArrayLength(cx, obj, &length))
			throw std::runtime_error("forEach: could not get array length");

		for (uint32_t i = 0; i < length; i++) {
			JS::RootedValue elem(cx);
			if (!JS_GetElement(cx, obj, i, &elem)) continue;
			auto data = std::make_shared<jValueData>(cx);
			data->val.set(elem);
			fn(jValue(ctx, data));
		}
	}

	void jValue::forEach(std::function<void(int, jValue)> fn) {
		JSContext* cx = ctx->ctx.get();
		auto v = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!v.isObject())
			throw std::runtime_error("forEach called on non-array");

		JS::RootedObject obj(cx, &v.toObject());
		bool isArray = false;
		JS::IsArrayObject(cx, obj, &isArray);
		if (!isArray)
			throw std::runtime_error("forEach called on non-array");

		uint32_t length = 0;
		if (!JS::GetArrayLength(cx, obj, &length))
			throw std::runtime_error("forEach: could not get array length");

		for (uint32_t i = 0; i < length; i++) {
			JS::RootedValue elem(cx);
			if (!JS_GetElement(cx, obj, i, &elem)) continue;
			auto data = std::make_shared<jValueData>(cx);
			data->val.set(elem);
			fn((int)i, jValue(ctx, data));
		}
	}

	void jValue::forEach(std::function<void(const std::string&, jValue)> fn) {
		JSContext* cx = ctx->ctx.get();
		auto v = resolvePath(ctx->ctx.get(), Data->val.get(), path);
		if (!v.isObject())
			throw std::runtime_error("forEach called on non-object");

		JS::RootedObject obj(cx, &v.toObject());
		bool isArray = false;
		JS::IsArrayObject(cx, obj, &isArray);
		if (isArray)
			throw std::runtime_error("forEach called on array, expected object");

		JS::RootedIdVector props(cx);
		if (!js::GetPropertyKeys(cx, obj, JSITER_OWNONLY, &props))
			throw std::runtime_error("forEach: could not get keys");

		for (size_t i = 0; i < props.length(); i++) {
			JS::RootedId id(cx, props[i]);
			JS::RootedValue idVal(cx);
			if (!JS_IdToValue(cx, id, &idVal)) continue;
			if (!idVal.isString()) continue;
			JS::RootedString str(cx, idVal.toString());
			JS::UniqueChars chars = JS_EncodeStringToUTF8(cx, str);
			std::string key(chars.get());

			JS::RootedValue elem(cx);
			if (!JS_GetProperty(cx, obj, key.c_str(), &elem)) continue;
			auto data = std::make_shared<jValueData>(cx);
			data->val.set(elem);
			fn(key, jValue(ctx, data));
		}
	}
}