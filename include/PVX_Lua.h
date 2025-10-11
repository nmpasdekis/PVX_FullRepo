#pragma once

#include <lua.hpp>
#include <memory>
#include <filesystem>
#include <string>
#include <PVX_File.h>
#include <optional>
#include <PVX_Json.h>
#include <type_traits>

namespace PVX::Scripting {
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
					out = static_cast<int64_t>(lua_tointeger(L, idx));
				else 
					out = static_cast<double>(lua_tonumber(L, idx));
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
				const auto& arr = v.Array(); // std::vector<Item>
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

	typedef int32_t (*LuaFunctionType)(lua_State*);

	template<class... R>
	class LuaFunc {
		lua_State* state;
		std::string name;

		inline void push(bool v) { lua_pushboolean(state, v); }
		inline void push(const char* s) { lua_pushstring(state, s); }
		inline void push(const std::string& s) { lua_pushlstring(state, s.c_str(), s.size()); }
		inline void push(const std::wstring& s) { auto s2 = PVX::Encode::UtfString(s); lua_pushlstring(state, s2.c_str(), s2.size()); }
		inline void push(double d) { lua_pushnumber(state, d); }
		inline void push(float f) { lua_pushnumber(state, static_cast<lua_Number>(f)); }
		inline void push(int32_t i) { lua_pushinteger(state, static_cast<lua_Integer>(i)); }
		inline void push(int64_t i) { lua_pushinteger(state, static_cast<lua_Integer>(i)); }

		template<class T> static T lua_read(lua_State* L, int32_t idx);

		//template<> inline static bool lua_read<bool>(lua_State* L, int32_t i) { if (!lua_isboolean(L, i)) throw std::runtime_error("expected bool"); return lua_toboolean(L, i) != 0; }
		//template<> inline static int32_t lua_read<int32_t>(lua_State* L, int32_t i) { if (!lua_isinteger(L, i)) throw std::runtime_error("expected int32_t"); return static_cast<int32_t>(lua_tointeger(L, i)); }
		//template<> inline static int64_t lua_read<int64_t>(lua_State* L, int32_t i) { if (!lua_isinteger(L, i)) throw std::runtime_error("expected integer"); return static_cast<int64_t>(lua_tointeger(L, i)); }
		//template<> inline static double lua_read<double>(lua_State* L, int32_t i) { if (!lua_isnumber(L, i)) throw std::runtime_error("expected number"); return static_cast<double>(lua_tonumber(L, i)); }
		//template<> inline static std::string lua_read<std::string>(lua_State* L, int32_t i) {
		//	if (!lua_isstring(L, i)) throw std::runtime_error("expected string");
		//	size_t len = 0; const char* s = lua_tolstring(L, i, &len); return std::string(s, len);
		//}

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
		return_type operator()(Args&&... args) {
			lua_getglobal(state, name.c_str());
			if (!lua_isfunction(state, -1))
				throw std::runtime_error(std::string("Lua function not found: ") + name);

			(push(std::forward<Args>(args)), ...);

			constexpr int32_t nrets = static_cast<int32_t>(sizeof...(R));
			if (lua_pcall(state, sizeof...(Args), nrets, 0) != LUA_OK) {
				std::string err = lua_tostring(state, -1);
				lua_pop(state, 1);
				throw std::runtime_error("lua_pcall error: " + err);
			}
			return readReturns_(std::index_sequence_for<R...>{});
		}
	};

	struct luaPathItem {
		bool isNumber;
		std::variant<int32_t, std::string> Index;
		inline void set(lua_State* L) {
			if (isNumber) lua_seti(L, -2, std::get<int32_t>(Index));
			else lua_setfield(L, -2, std::get<std::string>(Index).c_str());
		}
		inline luaPathItem(bool num, int32_t Index): isNumber{ num }, Index{ Index }{}
		inline luaPathItem(bool num, std::string Index) : isNumber{ num }, Index{ Index } {}
	};

	class LuaPath {
		lua_State* L;
		int32_t RefIndex;
		std::vector<luaPathItem> path;
		std::shared_ptr<int32_t> refCount;

		inline LuaPath(lua_State* L, int32_t regIndex, std::vector<luaPathItem> path, std::shared_ptr<int32_t> ref): L{ L }, RefIndex{ regIndex }, path{ path }, refCount{ ref } {};
		inline LuaPath(lua_State* L, int32_t regIndex, int32_t Index): L{ L }, RefIndex{ regIndex }, path{ std::vector<luaPathItem>{ luaPathItem{ true, Index } } }, refCount{ std::make_shared<int32_t>(1) } {};
		inline LuaPath(lua_State* L, int32_t regIndex, std::string Index) : L{ L }, RefIndex{ regIndex }, path{ std::vector<luaPathItem>{ luaPathItem{ false, Index } } }, refCount{ std::make_shared<int32_t>(1) } {};

		inline void pushPath() const {
			lua_rawgeti(L, LUA_REGISTRYINDEX, RefIndex);
			for (auto& p : path) {
				if (p.isNumber) lua_geti(L, -1, std::get<int32_t>(p.Index));
				else lua_getfield(L, -1, std::get<std::string>(p.Index).c_str());
				lua_replace(L, -2);
			}
		}
		inline const luaPathItem& pushPathMinusOne() const {
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
	public:
		inline LuaPath(const LuaPath& p){ 
			L = p.L;
			RefIndex = p.RefIndex;
			refCount = p.refCount;
			path = p.path;
			(*refCount)++;
		}
		inline LuaPath operator[](int32_t Index) {
			auto v = path;
			v.emplace_back(true, Index);
			(*refCount)++;
			return LuaPath{ L, RefIndex, v, refCount };
		}
		inline LuaPath operator[](std::string Index) {
			auto v = path;
			v.emplace_back(false, Index);
			(*refCount)++;
			return LuaPath{ L, RefIndex, v, refCount };
		}
		inline LuaPath operator[](const char * Index) {
			return operator[](std::string(Index));
		}
		inline ~LuaPath() {
			(*refCount)--;
			if(!*refCount) 
				luaL_unref(L, LUA_REGISTRYINDEX, RefIndex);
		}
		inline operator int64_t() const {
			pushPath();
			int64_t ret = lua_tointeger(L, -1);
			lua_pop(L, -1);
			return ret;
		}
		inline operator int32_t() const {
			pushPath();
			int32_t ret = (int32_t)lua_tointeger(L, -1);
			lua_pop(L, -1);
			return ret;
		}
		inline operator std::string() const {
			pushPath();
			std::string ret = lua_tostring(L, -1);
			lua_pop(L, -1);
			return ret;
		}
		inline operator bool() const {
			pushPath();
			bool ret = lua_toboolean(L, -1);
			lua_pop(L, -1);
			return ret;
		}
		inline int32_t operator=(int32_t val) {
			auto p = pushPathMinusOne();
			lua_pushinteger(L, val);
			p.set(L);
			return val;
		}
		inline int64_t operator=(int64_t val) {
			auto p = pushPathMinusOne();
			lua_pushinteger(L, val);
			p.set(L);
			return val;
		}
		inline bool operator=(bool val) {
			auto p = pushPathMinusOne();
			lua_pushboolean(L, val);
			p.set(L);
			return val;
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
		inline const char* operator=(const char* val) {
			auto p = pushPathMinusOne();
			lua_pushstring(L, val);
			p.set(L);
			return val;
		}
		inline const wchar_t* operator=(const wchar_t* val) {
			auto p = pushPathMinusOne();
			lua_pushstring(L, PVX::Encode::UtfString(val).c_str());
			p.set(L);
			return val;
		}
	};

	class LuaGlobal {
		lua_State* L;
		std::string name;
		inline LuaGlobal(lua_State* L, std::string name):L{ L }, name{name}{};


		friend class Lua;
	public:
		inline LuaPath operator[](int32_t Index) {
			lua_getglobal(L, name.c_str());
			auto ref = luaL_ref(L, LUA_REGISTRYINDEX);
			return LuaPath{ L, ref, Index };
		}
		inline LuaPath operator[](std::string Index) {
			lua_getglobal(L, name.c_str());
			auto ref = luaL_ref(L, LUA_REGISTRYINDEX);
			return LuaPath{ L, ref, Index };
		}
		inline LuaPath operator[](const char* Index) {
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
		inline const PVX::JSON::Item & operator=(const PVX::JSON::Item& v) {
			push_json(L, v);
			lua_setglobal(L, name.c_str());
			return v;
		}

		inline operator PVX::JSON::Item() {
			lua_getglobal(L, name.c_str());
			return read_json(L, -1);
		}

		inline void operator=(LuaFunctionType f) {
			lua_pushcclosure(L, f, 0);
			lua_setglobal(L, name.c_str());
		}

		template<class... R>
		LuaFunc<R...> func() {
			return LuaFunc<R...>{ L, std::move(name) };
		}
	};



	class Lua {
		lua_State* L;
	public:
		inline ~Lua() { lua_close(L); }
		inline Lua(): L { luaL_newstate() }{ luaL_openlibs(L); }
		inline void doString(const std::string& code) {
			auto rez = luaL_dostring(L, code.c_str());
		}
		inline void doFile(const std::filesystem::path& Filename) {
			auto code = PVX::IO::ReadText(Filename);
			luaL_dostring(L, code.c_str());
		}
		/*inline std::optional<int32_t> getInt32(const std::string& name) {
			lua_getglobal(L, name.c_str());
			int32_t ok = 0;
			auto ret = lua_tointegerx(L, -1, &ok);
			if(ok) return (int32_t)ret;
			return std::optional<int32_t>{};
		}
		inline std::optional<int64_t> getInt64(const std::string& name) {
			lua_getglobal(L, name.c_str());
			int32_t ok = 0;
			auto ret = lua_tointegerx(L, -1, &ok);
			if (ok) return (int64_t)ret;
			return std::optional<int64_t>{};
		}
		inline int64_t get(const std::string& name, int64_t def) {
			lua_getglobal(L, name.c_str());
			int32_t ok = 0;
			lua_Integer ret = lua_tointegerx(L, -1, &ok);
			if (ok) return (int64_t)ret;
			return def;
		}
		inline int32_t get(const std::string& name, int32_t def = 0) {
			lua_getglobal(L, name.c_str());
			int32_t ok = 0;
			lua_Integer ret = lua_tointegerx(L, -1, &ok);
			if (ok) return (int32_t)ret;
			return def;
		}*/
		inline LuaGlobal operator[](std::string name) {
			return LuaGlobal(L, name);
		}
		template<class... R>
		LuaFunc<R...> func(std::string name) {
			return LuaFunc<R...>{ L, std::move(name) };
		}
	};
}