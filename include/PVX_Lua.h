#pragma once

#include <lua.hpp>
#include <memory>
#include <filesystem>
#include <string>
#include <PVX_File.h>
#include <optional>
#include <PVX_Json.h>
#include <type_traits>
#include <variant>


#define PVX_LuaOpReturn2(retType, fnc) pushPath();\
	retType ret = fnc(L, -1);\
	lua_pop(L, -1);\
	return ret;
#define PVX_LuaOpReturn(fnc) pushPath();\
	auto ret = fnc(L, -1);\
	lua_pop(L, -1);\
	return ret;
#define PVX_LuaSetValue(fnc) auto p = pushPathMinusOne();\
	fnc(L, val);\
	p.set(L);\
	return val;


namespace PVX::Scripting {
	class LuaParam;
	class LuaGlobal;
	class LuaReturner;
	class LuaDictionary;
	class LuaArray;

	using LuaParams = std::vector<LuaParam>;
	using luaCppFunction = std::function<void(const LuaParams& Params, LuaReturner&)>;
	static int callback(lua_State* L);

	enum class LuaBool {
		False = false,
		True = true
	};

	std::string preprocess(std::string fn);

	template<typename T>
	class myContainer {
		std::vector<std::unique_ptr<T>> Store;
		std::vector<size_t> freelist;
	public:
		std::tuple<size_t, T*> add(T&& t) {
			if(!freelist.empty()) {
				auto id = freelist.back();
				freelist.pop_back();
				auto & ret = Store[id];// = t;
				ret = std::make_unique<T>(t);
				return std::tuple<size_t, T*>{ id, ret.get() };
			} else {
				auto id = Store.size();
				auto& ret = Store.emplace_back(std::make_unique<T>(t));
				return std::tuple<size_t, T*>{ id, ret.get() };
			}
		}
		void remove(size_t id) {
			Store[id].reset();
			freelist.push_back(id);
		}
	};

	extern myContainer<luaCppFunction> myFunctions;

	namespace{
		template<typename T>
		class implicit {
			std::shared_ptr<T> data;
		public:
			inline implicit(const T & v): data{ std::make_shared<T>(v) } {}
			inline T& get() const { return *data.get(); }
		};
		using luaVarVariant = std::variant<nullptr_t, LuaBool, int32_t, int64_t, float, double, std::string, implicit<LuaDictionary>, implicit<LuaArray>, luaCppFunction>;
		struct luaDictPair {
			std::string key;
			luaVarVariant value;
		};
		struct FuncUpvalue {
			luaCppFunction* pFnc;
			int32_t id;
		};

		inline void push(lua_State* L, nullptr_t) { lua_pushnil(L); }
		inline void push(lua_State* L, LuaBool v) { lua_pushboolean(L, (bool)v); }
		inline void push(lua_State* L, const char* s) { lua_pushstring(L, s); }
		inline void push(lua_State* L, const std::string& s) { lua_pushlstring(L, s.c_str(), s.size()); }
		inline void push(lua_State* L, const std::wstring& s) { auto s2 = PVX::Encode::UtfString(s); lua_pushlstring(L, s2.c_str(), s2.size()); }
		inline void push(lua_State* L, double d) { lua_pushnumber(L, d); }
		inline void push(lua_State* L, float f) { lua_pushnumber(L, static_cast<lua_Number>(f)); }
		inline void push(lua_State* L, int32_t i) { lua_pushinteger(L, static_cast<lua_Integer>(i)); }
		inline void push(lua_State* L, int64_t i) { lua_pushinteger(L, static_cast<lua_Integer>(i)); }
		inline luaCppFunction* push(lua_State* L, luaCppFunction f) {
			auto [id, ret] = myFunctions.add(std::forward<luaCppFunction>(f));
			auto* ud = static_cast<FuncUpvalue*>(lua_newuserdatauv(L, sizeof(FuncUpvalue), 0));
			ud->pFnc = ret;
			ud->id = id;
			lua_getfield(L, LUA_REGISTRYINDEX, "fncGC");
			lua_setmetatable(L, -2);
			lua_pushcclosure(L, &callback, 1);
			return ret;
		}
	}

	class LuaArray {
		std::vector<luaVarVariant> array;
	public:
		inline LuaArray(const std::initializer_list<luaVarVariant>& list) {
			for(const auto&l : list) {
				array.push_back(l);
			}
		}
		inline const std::vector<luaVarVariant>& get() const { return array; }
	};
	class LuaDictionary {
		std::unordered_map<std::string, luaVarVariant> dict;
	public:
		inline LuaDictionary(const std::initializer_list<luaDictPair> & list) {
			for(const auto& l : list) {
				dict.emplace(l.key, l.value);
				//dict[l.key] = l.value;
			}
		}
		inline const std::unordered_map<std::string, luaVarVariant>& get() const { return dict; }
	};

	static inline std::vector<std::string> getKeys(lua_State* L);
	static inline std::vector<LuaParam> getValues(lua_State* L);
	static inline std::vector<std::tuple<std::string, LuaParam>> getKeysAndValues(lua_State* L);
	namespace {
		inline bool is_array_table(lua_State* L, int32_t idx) {
			idx = lua_absindex(L, idx);

			lua_len(L, idx);
			const lua_Integer n = lua_tointeger(L, -1);
			lua_pop(L, 1);

			// verify only integer keys in [1..n]
			bool ok = true;
			int32_t nonArrKeys = 0;

			lua_pushnil(L);                 // first key
			while (lua_next(L, idx) != 0) { // stack: ... key at -2, value at -1
				if (!lua_isinteger(L, -2)) { nonArrKeys++; }
				else {
					const lua_Integer k = lua_tointeger(L, -2);
					if (k < 1 || k > n) ok = false;
				}
				lua_pop(L, 1); // pop value, keep key
				if (!ok) break;
			}
			if (!ok || nonArrKeys) return false;

			// ensure 1..n have values (no holes)
			for (lua_Integer i = 1; i <= n; ++i) {
				lua_rawgeti(L, idx, i);
				const bool exists = !lua_isnil(L, -1);
				lua_pop(L, 1);
				if (!exists) return false;
			}
			return true;
		}
		inline PVX::JSON::Item read_json(lua_State* L, int32_t idx) {
			idx = lua_absindex(L, idx);
			const int32_t t = lua_type(L, idx);

			PVX::JSON::Item out = PVX::JSON::jsElementType::Undefined;

			switch (t) {
			case LUA_TNIL: {
				return out;
			}
			case LUA_TBOOLEAN: {
				out = (lua_toboolean(L, idx) != 0);
				return out;
			}
			case LUA_TNUMBER: {
				if (lua_isinteger(L, idx))
					out = lua_tointeger(L, idx);
				else
					out = lua_tonumber(L, idx);
				return out;
			}
			case LUA_TSTRING: {
				size_t len = 0;
				const char* s = lua_tolstring(L, idx, &len);
				out = PVX::Decode::UTF(s);
				return out;
			}
			case LUA_TTABLE: {
				if (is_array_table(L, idx)) {
					lua_len(L, idx);
					const lua_Integer n = lua_tointeger(L, -1);
					lua_pop(L, 1);

					std::vector<PVX::JSON::Item> arr;
					arr.reserve(static_cast<std::size_t>(n));
					for (lua_Integer i = 1; i <= n; ++i) {
						lua_rawgeti(L, idx, i);
						arr.emplace_back(read_json(L, -1));
						lua_pop(L, 1);
					}
					out = std::move(arr);
					return out;
				}
				else {
					std::unordered_map<std::wstring, PVX::JSON::Item> mp;

					lua_pushnil(L);
					while (lua_next(L, idx) != 0) {
						std::wstring key;
						if (lua_type(L, -2) == LUA_TSTRING) {
							size_t klen = 0;
							const char* ks = lua_tolstring(L, -2, &klen);
							key = PVX::Decode::UTF(std::string{ ks, klen });
						}
						else if (lua_isinteger(L, -2)) {
							key = std::to_wstring(static_cast<int64_t>(lua_tointeger(L, -2)));
						}
						else {
							luaL_typename(L, -2);
							read_json(L, -1);
							lua_pop(L, 1);
							continue;
						}

						mp.emplace(std::move(key), read_json(L, -1));
						lua_pop(L, 1);
					}
					out = std::move(mp);
					return out;
				}
			}
			default:
				return out;
			}
		}
		inline void push_json(lua_State* state, const PVX::JSON::Item& v) {
			switch (v.Type()) {
			case PVX::JSON::jsElementType::Null:
			case PVX::JSON::jsElementType::Undefined: lua_pushnil(state); return;
			case PVX::JSON::jsElementType::Boolean: lua_pushboolean(state, v.Boolean()); return;
			case PVX::JSON::jsElementType::Integer: lua_pushinteger(state, static_cast<lua_Integer>(v.Integer())); return;
			case PVX::JSON::jsElementType::Float: lua_pushnumber(state, static_cast<lua_Number>(v.Double())); return;
			case PVX::JSON::jsElementType::String: { const std::string s = PVX::Encode::UtfString(v.String()); lua_pushlstring(state, s.data(), s.size()); } return;
			case PVX::JSON::jsElementType::Array: {
				const auto& arr = v.Array();
				lua_createtable(state, static_cast<int32_t>(arr.size()), 0);
				for (std::size_t i = 0; i < arr.size(); ++i) {
					push_json(state, arr[i]);
					lua_rawseti(state, -2, static_cast<lua_Integer>(i + 1));
				}
				return;
			}
			case PVX::JSON::jsElementType::Object: {
				const auto& obj = v.Object(); // std::unordered_map<std::wstring, Item>
				lua_createtable(state, 0, static_cast<int32_t>(obj.size()));
				for (const auto& kv : obj) {
					const std::string key = PVX::Encode::UtfString(kv.first);
					lua_pushlstring(state, key.data(), key.size());
					push_json(state, kv.second);
					lua_settable(state, -3); // t[key] = val
				}
				return;
			}
			}
		}
	}

	namespace {
		void push(lua_State*L, const luaVarVariant& v) {
			switch(v.index()) {
			case 0: push(L, nullptr); break;
			case 1: push(L, std::get<1>(v)); break;
			case 2: push(L, std::get<2>(v)); break;
			case 3: push(L, std::get<3>(v)); break;
			case 4: push(L, std::get<4>(v)); break;
			case 5: push(L, std::get<5>(v)); break;
			case 6: push(L, std::get<6>(v)); break;
			case 7:
			{
				auto& dict = std::get<7>(v).get().get();
				lua_createtable(L, 0, dict.size());
				for(const auto&[name, value] : dict) {
					push(L, value);
					lua_setfield(L, -2, name.c_str());
				}
			} break;
			case 8:
			{
				auto& dict = std::get<8>(v).get().get();
				lua_createtable(L, dict.size(), 0);
				int i = 1;
				for(const auto& value : dict) {
					push(L, value);
					lua_seti(L, -2, i++);
				}
			}
			break;
			case 9: push(L, std::get<9>(v)); break;
			}
		}
	}

	typedef int32_t (*LuaFunctionType)(lua_State*);

	template<class... R>
	class LuaFunc {
		lua_State* state;
		std::string name;

		template<class T> static T lua_read(lua_State* L, int32_t idx);

		template<> inline static bool lua_read<bool>(lua_State* L, int32_t i) { return lua_toboolean(L, i) != 0; }
		template<> inline static int32_t lua_read<int32_t>(lua_State* L, int32_t i) { return static_cast<int32_t>(lua_tointeger(L, i)); }
		template<> inline static int64_t lua_read<int64_t>(lua_State* L, int32_t i) { return static_cast<int64_t>(lua_tointeger(L, i)); }
		template<> inline static double lua_read<double>(lua_State* L, int32_t i) { return static_cast<double>(lua_tonumber(L, i)); }
		template<> inline static std::string lua_read<std::string>(lua_State* L, int32_t i) { return lua_tostring(L, i); }
		template<> inline static std::wstring lua_read<std::wstring>(lua_State* L, int32_t i) { return PVX::Decode::UTF(lua_tostring(L, i)); }

		template<class... R> struct RetType { using type = std::tuple<R...>; };
		template<class R>    struct RetType<R> { using type = R; };
		template<>           struct RetType<> { using type = void; };

		using return_type = typename RetType<R...>::type;
		return_type readReturns_(std::index_sequence<>) const {
			return;
		}

		template<std::size_t... I>
		return_type readReturns_(std::index_sequence<I...>) const {
			constexpr int32_t N = static_cast<int32_t>(sizeof...(R));
			auto tup = std::tuple<R...>(
				lua_read<R>(state, static_cast<int32_t>(I) - N)...
			);
			lua_pop(state, static_cast<int32_t>(sizeof...(R)));
			if constexpr (sizeof...(R) == 1) {
				return std::get<0>(tup);
			}
			else {
				return tup;
			}
		}
		friend class LuaGlobal;
		friend class Lua;

		inline LuaFunc<R...>(lua_State* state, std::string name) :state{ state }, name{ name } {};
	public:
		template< typename... Args>
		return_type operator()(Args&&... args) const {
			lua_getglobal(state, name.c_str());
			if (!lua_isfunction(state, -1))
				throw std::runtime_error(std::string("Lua function not found: ") + name);

			(push(state, std::forward<Args>(args)), ...);

			constexpr int32_t nrets = static_cast<int32_t>(sizeof...(R));
			if (lua_pcall(state, sizeof...(Args), nrets, 0) != LUA_OK) {
				std::string err = lua_tostring(state, -1);
				lua_pop(state, 1);
				throw std::runtime_error("lua_pcall error: " + err);
			}
			return readReturns_(std::index_sequence_for<R...>{});
		}
	};

	struct LuaPathItem {
		bool isNumber;
		std::variant<int32_t, std::string> Index;
		inline void set(lua_State* L) {
			if (isNumber) lua_seti(L, -2, std::get<int32_t>(Index));
			else lua_setfield(L, -2, std::get<std::string>(Index).c_str());
		}
		inline LuaPathItem(bool num, int32_t Index): isNumber{ num }, Index{ Index }{}
		inline LuaPathItem(bool num, std::string Index) : isNumber{ num }, Index{ Index } {}
	};

	class LuaParam {
		lua_State* L;
		int32_t RefIndex;
		std::vector<LuaPathItem> path;
		std::shared_ptr<int32_t> refCount;

		inline LuaParam(lua_State* L, int32_t regIndex, std::vector<LuaPathItem> path, std::shared_ptr<int32_t> ref): L{ L }, RefIndex{regIndex}, path{path}, refCount{ref} {};
		inline LuaParam(lua_State* L, int32_t regIndex, int32_t Index): L{ L }, RefIndex{ regIndex }, path{ std::vector<LuaPathItem>{ LuaPathItem{ true, Index } } }, refCount{ std::make_shared<int32_t>(1) } {};
		inline LuaParam(lua_State* L, int32_t regIndex, std::string Index) : L{ L }, RefIndex{ regIndex }, path{ std::vector<LuaPathItem>{ LuaPathItem{ false, Index } } }, refCount{ std::make_shared<int32_t>(1) } {};

		inline void pushPath() const {
			lua_rawgeti(L, LUA_REGISTRYINDEX, RefIndex);
			for (auto& p : path) {
				if (p.isNumber) lua_geti(L, -1, std::get<int32_t>(p.Index));
				else lua_getfield(L, -1, std::get<std::string>(p.Index).c_str());
				lua_replace(L, -2);
			}
		}
		inline const LuaPathItem& pushPathMinusOne() const {
			lua_rawgeti(L, LUA_REGISTRYINDEX, RefIndex);
			for(auto i = 0; i < path.size() - 1; i++) {
				auto&p = path[i];
				if (p.isNumber) lua_geti(L, -1, std::get<int32_t>(p.Index));
				else lua_getfield(L, -1, std::get<std::string>(p.Index).c_str());
				lua_replace(L, -2);
			}
			return path.back();
		}

		friend class LuaGlobal;
		friend class Lua;
		friend int callback(lua_State* L);
		friend inline std::vector<std::string> getKeys(lua_State* L);
		friend inline std::vector<LuaParam> getValues(lua_State* L);
		friend inline std::vector<std::tuple<std::string, LuaParam>> getKeysAndValues(lua_State* L);
	public:
		inline LuaParam(lua_State* L, int32_t regIndex) : L{ L }, RefIndex{ regIndex }, refCount{std::make_shared<int32_t>(1)} {}
		inline LuaParam(const LuaParam& p) : L{ p.L }, RefIndex{ p.RefIndex }, refCount{ p.refCount }, path{ p.path } { (*refCount)++; }
		inline LuaParam operator[](int32_t Index) const {
			auto v = path;
			v.emplace_back(true, Index);
			(*refCount)++;
			return LuaParam{ L, RefIndex, v, refCount };
		}
		inline LuaParam operator[](std::string Index) const {
			auto v = path;
			v.emplace_back(false, Index);
			(*refCount)++;
			return LuaParam{ L, RefIndex, v, refCount };
		}
		inline LuaParam operator[](const char* Index) const {
			auto v = path;
			v.emplace_back(false, Index);
			(*refCount)++;
			return LuaParam{ L, RefIndex, v, refCount };
		}
		inline LuaParam operator[](std::string && Index) && {
			path.emplace_back(false, Index);
			return std::move(*this);
		}
		inline LuaParam operator[](const char* && Index) && {
			path.emplace_back(false, Index);
			return std::move(*this);
		}
		inline LuaParam operator[](int32_t && Index) && {
			path.emplace_back(true, Index);
			return std::move(*this);
		}
		inline ~LuaParam() {
			(*refCount)--;
			if(!*refCount) 
				luaL_unref(L, LUA_REGISTRYINDEX, RefIndex);
		}

		inline int64_t length() const {
			pushPath();
			lua_len(L, -1);
			auto len = lua_tointeger(L, -1);
			lua_pop(L, 2);
			return len;
		}
		inline std::vector<std::string> keys() const {
			pushPath();
			auto ret = getKeys(L);
			lua_pop(L, 1);
			return ret;
		}
		inline std::vector<LuaParam> values() const {
			pushPath();
			return getValues(L);
		}
		inline std::vector<std::tuple<std::string, LuaParam>> keyValuesPairs() const {
			pushPath();
			return getKeysAndValues(L);
		}

		inline operator int64_t() const {
			PVX_LuaOpReturn(lua_tointeger);
		}
		inline operator int32_t() const {
			PVX_LuaOpReturn2(int32_t, lua_tointeger);
		}
		inline operator std::string() const {
			PVX_LuaOpReturn2(std::string, lua_tostring);
		}
		inline operator bool() const {
			PVX_LuaOpReturn(lua_toboolean);
		}
		inline operator PVX::JSON::Item() const {
			PVX_LuaOpReturn(read_json);
		}
		inline int32_t operator=(int32_t val) {
			PVX_LuaSetValue(lua_pushinteger);
		}
		inline int64_t operator=(int64_t val) {
			PVX_LuaSetValue(lua_pushinteger);
		}
		inline const PVX::JSON::Item& operator=(PVX::JSON::Item&& val) {
			PVX_LuaSetValue(push_json);
		}
		inline bool operator=(LuaBool val) {
			auto p = pushPathMinusOne();
			lua_pushboolean(L, (bool)val);
			p.set(L);
			return (bool)val;
		}
		inline bool setBool(bool val) {
			PVX_LuaSetValue(lua_pushboolean);
		}
		inline const char* operator=(const char* val) {
			PVX_LuaSetValue(lua_pushstring);
		}
		inline nullptr_t operator=(nullptr_t) {
			auto p = pushPathMinusOne();
			lua_pushnil(L);
			p.set(L);
			return nullptr;
		}
		inline std::string operator=(std::string val) {
			auto p = pushPathMinusOne();
			lua_pushstring(L, val.c_str());
			p.set(L);
			return val;
		}
		inline std::wstring operator=(std::wstring val) {
			auto p = pushPathMinusOne();
			lua_pushstring(L, PVX::Encode::UtfString(val).c_str());
			p.set(L);
			return val;
		}
		inline const wchar_t* operator=(const wchar_t* val) {
			auto p = pushPathMinusOne();
			lua_pushstring(L, PVX::Encode::UtfString(val).c_str());
			p.set(L);
			return val;
		}
		inline LuaParam& operator=(LuaParam&& val) noexcept {
			auto p = pushPathMinusOne();
			val.pushPath();
			p.set(L);
			return *this;
		}

		inline void operator=(LuaFunctionType f) {
			auto p = pushPathMinusOne();
			lua_pushcclosure(L, f, 0);
			p.set(L);
		}
		inline auto& operator=(luaCppFunction&& f) {
			auto p = pushPathMinusOne();
			auto pRet = push(L, std::forward<luaCppFunction>(f));
			p.set(L);
			return *pRet;
		}
	};

	class LuaGlobal {
		lua_State* L;
		std::string name;
		inline LuaGlobal(lua_State* L, std::string name):L{ L }, name{name} {};

		friend class Lua;

	public:
		inline LuaParam operator[](int32_t Index) {
			lua_getglobal(L, name.c_str());
			auto ref = luaL_ref(L, LUA_REGISTRYINDEX);
			return LuaParam{ L, ref, Index };
		}
		inline LuaParam operator[](std::string Index) {
			lua_getglobal(L, name.c_str());
			auto ref = luaL_ref(L, LUA_REGISTRYINDEX);
			return LuaParam{ L, ref, Index };
		}
		inline LuaParam operator[](const char* Index) {
			return operator[](std::string(Index));
		}

		inline operator int64_t() {
			lua_getglobal(L, name.c_str());
			auto ret = lua_tointeger(L, -1);
			lua_pop(L, -1);
			return ret;
		}
		inline operator double() {
			lua_getglobal(L, name.c_str());
			auto ret = lua_tonumber(L, -1);
			lua_pop(L, -1);
			return ret;
		}
		inline operator float() {
			lua_getglobal(L, name.c_str());
			auto ret = lua_tonumber(L, -1);
			lua_pop(L, -1);
			return ret;
		}
		inline operator std::string() {
			lua_getglobal(L, name.c_str());
			std::string ret = lua_tostring(L, -1);
			lua_pop(L, -1);
			return ret;
		}
		inline operator std::wstring() {
			lua_getglobal(L, name.c_str());
			std::wstring ret = PVX::Decode::UTF(lua_tostring(L, -1));
			lua_pop(L, -1);
			return ret;
		}
		inline int64_t operator=(int64_t v) {
			lua_pushnumber(L, v);
			lua_setglobal(L, name.c_str());
			return v;
		}
		inline std::string operator=(std::string v) {
			lua_pushstring(L, v.c_str());
			lua_setglobal(L, name.c_str());
			return v;
		}
		inline const char* operator=(const char* v) {
			lua_pushstring(L, v);
			lua_setglobal(L, name.c_str());
			return v;
		}
		inline nullptr_t operator=(nullptr_t v) {
			lua_pushnil(L);
			lua_setglobal(L, name.c_str());
			return nullptr;
		}

		inline void operator=(LuaFunctionType f) {
			lua_pushcclosure(L, f, 0);
			lua_setglobal(L, name.c_str());
		}
		inline auto& operator=(luaCppFunction && f) {
			auto pRet = push(L, std::forward<luaCppFunction>(f));
			lua_setglobal(L, name.c_str());
			return *pRet;
		}
		inline LuaParam operator=(const LuaParam& v) {
			v.pushPath();
			lua_setglobal(L, name.c_str());
			return v;
		}
		inline const luaVarVariant& operator=(const luaVarVariant& v) {
			push(L, v);
			lua_setglobal(L, name.c_str());
			return v;
		}

		inline bool setBool(bool v) {
			lua_pushboolean(L, v);
			lua_setglobal(L, name.c_str());
			return v;
		}
		inline bool operator=(LuaBool v) {
			lua_pushboolean(L, (bool)v);
			lua_setglobal(L, name.c_str());
			return (bool)v;
		}

		inline const PVX::JSON::Item & operator=(const PVX::JSON::Item& v) {
			push_json(L, v);
			lua_setglobal(L, name.c_str());
			return v;
		}

		inline operator PVX::JSON::Item() {
			lua_getglobal(L, name.c_str());
			return read_json(L, -1);
		}


		inline std::vector<std::string> keys() const {
			lua_getglobal(L, name.c_str());
			auto ret = getKeys(L);
			lua_pop(L, 1);
			return ret;
		}
		inline std::vector<LuaParam> values() const {
			lua_getglobal(L, name.c_str());
			return getValues(L);
		}
		inline std::vector<std::tuple<std::string, LuaParam>> keyValuesPairs() const {
			lua_getglobal(L, name.c_str());
			return getKeysAndValues(L);
		}

		template<class... R>
		LuaFunc<R...> func() {
			return LuaFunc<R...>{ L, std::move(name) };
		}
	};

	class Lua {
		lua_State* L;
		std::unordered_map<std::string, int32_t> RegistryData;
	public:
		inline ~Lua() { lua_close(L); }
		inline Lua(): L { luaL_newstate() } { 
			luaL_openlibs(L);
			LuaFunctionType destroyer = [](lua_State* L) -> int {
				auto id = static_cast<FuncUpvalue*>(lua_touserdata(L, -1))->id;
				myFunctions.remove(id);
				return 0;
			};
			lua_createtable(L, 0, 2);
			lua_pushcclosure(L, destroyer, 0);
			lua_setfield(L, -2, "__gc");
			lua_pushstring(L, "The Destroyer");
			lua_setfield(L, -2, "__metatable");
			lua_setfield(L, LUA_REGISTRYINDEX, "fncGC");
		}
		inline bool doString(const std::string& code) {
			if (luaL_dostring(L, code.c_str()) != LUA_OK) {
				const char* msg = lua_tostring(L, -1);
				if (!msg) {
					luaL_tolstring(L, -1, NULL);
					msg = lua_tostring(L, -1);
					lua_remove(L, -2);
				}
				printf("Lua error: %s\n\n%s\n\n", msg ? msg : "(unknown error)", code.c_str());
				lua_pop(L, 1);
				return true;
			}
			return false;
		}
		inline bool doFile(const std::filesystem::path& Filename) {
			auto code = PVX::IO::ReadText(Filename);
			return doString(code);
		}
		inline bool doStringPP(const std::string& code) {
			return doString(preprocess(code));
		}
		inline bool doFilePP(const std::filesystem::path& Filename) {
			auto code = preprocess(PVX::IO::ReadText(Filename));
			return doString(code);
		}
		inline LuaGlobal operator[](std::string name) {
			return LuaGlobal(L, std::move(name));
		}
		inline LuaParam Registry(std::string Name) {
			return LuaParam{ L, LUA_REGISTRYINDEX, Name };
		}
		template<class... R>
		LuaFunc<R...> func(std::string name) const {
			return LuaFunc<R...>{ L, std::move(name) };
		}
	};


	namespace {
		using luaReturnVariant = std::variant<bool, int32_t, int64_t, double, std::string, luaCppFunction, PVX::JSON::Item, nullptr_t, luaVarVariant>;
	}

	class LuaReturner {
		lua_State* L;
		std::vector<luaReturnVariant> rets;
		friend int callback(lua_State* L);
	public:
		LuaReturner(lua_State* L) :L{ L } {}
		void operator<<(bool v) { rets.push_back(v); }
		void operator<<(int32_t v) { rets.push_back(v); }
		void operator<<(int64_t v) { rets.push_back(v); }
		void operator<<(double v) { rets.push_back(v); }
		void operator<<(std::string v) { rets.push_back(v); }
		void operator<<(const char* v) { rets.push_back(std::string(v)); }
		void operator<<(luaCppFunction v) { rets.push_back(v); }
		void operator<<(PVX::JSON::Item v) { rets.push_back(v); }
		void operator<<(nullptr_t) { rets.push_back(nullptr); }
		void operator<<(const luaVarVariant& v) { rets.push_back(v); }
	};


	static int callback(lua_State* L) {
		auto fnc_upvalue = lua_touserdata(L, lua_upvalueindex(1));
		auto& handler = *static_cast<FuncUpvalue*>(fnc_upvalue)->pFnc;
		int argc = lua_gettop(L);
		std::vector<int32_t> refs;
		refs.reserve(argc);
		std::vector<LuaParam> params;
		params.reserve(argc);
		for (auto i = 0; i < argc; i++) refs.push_back(luaL_ref(L, LUA_REGISTRYINDEX));
		for (auto i = argc - 1; i >= 0; i--) params.emplace_back(L, refs[i]);

		auto ret = LuaReturner(L);
		handler(params, ret);
		for (auto& r : ret.rets) {
			switch (r.index()) {
				case 0: lua_pushboolean(L, std::get<bool>(r)); break;
				case 1: lua_pushinteger(L, std::get<int32_t>(r)); break;
				case 2: lua_pushinteger(L, std::get<int64_t>(r)); break;
				case 3: lua_pushnumber(L, std::get<double>(r)); break;
				case 4: lua_pushstring(L, std::get<std::string>(r).c_str()); break;
				case 5: push(L, std::get<luaCppFunction>(r)); break;
				case 6: push_json(L, std::get<PVX::JSON::Item>(r)); break;
				case 7: lua_pushnil(L); break;
				case 8: push(L, std::get<8>(r)); break;
			}
		}
		return ret.rets.size();
	}

	static inline std::vector<std::string> getKeys(lua_State* L) {
		std::vector<std::string> ret;
		auto idx = lua_absindex(L, -1);
		lua_pushnil(L);
		while (lua_next(L, idx)) {
			if (lua_type(L, -2) == LUA_TSTRING) {
				ret.push_back(lua_tostring(L, -2));
			}
			else if (lua_isinteger(L, -2)) {
				ret.push_back(std::to_string(lua_tointeger(L, -2)));
			}
			lua_pop(L, 1);
		}
		return ret;
	}
	static inline std::vector<LuaParam> getValues(lua_State* L) {
		auto keys = getKeys(L);
		auto ref = luaL_ref(L, LUA_REGISTRYINDEX);
		auto p = LuaParam(L, ref);
		std::vector<LuaParam> ret;
		ret.reserve(keys.size());
		for (auto i = keys.size(); i > 0; i--) {
			ret.push_back(p[keys[i-1]]);
		}
		return ret;
	}
	static inline std::vector<std::tuple<std::string, LuaParam>> getKeysAndValues(lua_State* L) {
		auto keys = getKeys(L);
		auto ref = luaL_ref(L, LUA_REGISTRYINDEX);
		auto p = LuaParam(L, ref);
		std::vector<std::tuple<std::string, LuaParam>> ret;
		ret.reserve(keys.size());
		for (auto i = keys.size(); i > 0; i--) {
			ret.push_back(std::tuple<std::string, LuaParam>(keys[i-1], p[keys[i - 1]]));
		}
		return ret;
	}
}