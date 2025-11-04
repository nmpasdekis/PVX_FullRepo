#pragma once

#include <variant>
#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include "PVX_json.h"

namespace PVX::Script {
	class Variant {
		enum class Type {
			Null,
			Boolean,
			Integer,
			Float,
			String,
			Object,
			Array,
			Function,
			ScopeDef
		};
		void* Data;
		Type tp;
		inline void Release() {
			switch (tp) {
				case Type::Boolean: delete ((bool*)Data);	break;
				case Type::Integer: delete ((int64_t*)Data); break;
				case Type::Float: delete ((double*)Data); break;
				case Type::String: delete ((std::wstring*)Data); break;
				case Type::Object: delete ((std::unordered_map<std::wstring, Variant>*)Data); break;
				case Type::Array: delete ((std::vector<Variant>*)Data); break;
				case Type::ScopeDef: delete ((std::unordered_map<std::wstring, Variant>*)Data); break;
				//case Type::Function: delete ((bool*)Data); break;
			}
		}
	public:
		~Variant() { Release(); }
		Variant() = default;
		Variant(const int v) :tp{ Type::Integer }, Data{ new int64_t { v } } {}
		Variant(const int64_t v) :tp{ Type::Integer }, Data{ new int64_t { v } } {}
		Variant(const float v) :tp{ Type::Float }, Data{ new double { v } } {}
		Variant(const double v) :tp{ Type::Float }, Data{ new double { v } } {}
		Variant(const bool v) :tp{ Type::Boolean }, Data{ new bool { v } } {}
		Variant(const wchar_t* v) :tp{ Type::String }, Data{ new std::wstring { v } } {}
		Variant(const std::wstring& v) :tp{ Type::String }, Data{ new std::wstring { v } } {}
		Variant(const std::unordered_map<std::wstring, Variant>& v) :tp{ Type::Object }, Data{ new std::unordered_map<std::wstring, Variant>{ v } } {}
		Variant(const std::vector<Variant>& v) :tp{ Type::Array }, Data{ new std::vector<Variant> { v } } {}
		Variant(const std::nullptr_t v) :tp{ Type::Null }, Data{ nullptr } {}

		inline Variant& operator=(const int v) { Release(); tp = Type::Integer; Data = new int64_t{ v }; return *this; };
		inline Variant& operator=(const int64_t v) { Release(); tp = Type::Integer; Data = new int64_t{ v }; return *this; };
		inline Variant& operator=(const double v) { Release(); tp = Type::Float; Data = new double{ v }; return *this; };
		inline Variant& operator=(const float v) { Release(); tp = Type::Float; Data = new double{ v }; return *this; };
		inline Variant& operator=(const bool v) { Release(); tp = Type::Boolean; Data = new bool{ v }; return *this; };
		inline Variant& operator=(const std::wstring v) { Release(); tp = Type::String; Data = new std::wstring{ v }; return *this; };
		inline Variant& operator=(const std::unordered_map<std::wstring, Variant>& v) { Release(); tp = Type::Integer; Data = new std::unordered_map<std::wstring, Variant>{ v }; return *this; };
		inline Variant& operator=(const std::vector<Variant>& v) { Release(); tp = Type::Integer; Data = new std::vector<Variant>{ v }; return *this; };
		inline Variant& operator=(const std::nullptr_t v) { Release(); tp = Type::Integer; Data =nullptr; return *this; };

		template<typename T> T& Get() { return *(T*)Data; }
		template<typename T> T& Get() const { return *(T*)Data; }
		inline Type GetType() { return tp; }
		inline int64_t ToInteger() const {
			switch (tp) {
				case Type::Boolean: return (*(bool*)Data);
				case Type::Integer: return (*(int64_t*)Data);
				case Type::Float: return (*(double*)Data);
				case Type::String: return std::stoi((*(std::wstring*)Data).c_str());
				default: return 0;
			}
		}
		inline double ToFloat() const {
			switch (tp) {
				case Type::Boolean: return (*(bool*)Data);
				case Type::Integer: return (*(int64_t*)Data);
				case Type::Float: return (*(double*)Data);
				case Type::String: return std::stod((*(std::wstring*)Data).c_str());
				default: return 0;
			}
		}
		inline std::wstring ToString() const {
			switch (tp) {
				case Type::Boolean: return ((*(bool*)Data)) ? L"true" : L"false";
				case Type::Integer: return std::to_wstring(*(int64_t*)Data);
				case Type::Float: return std::to_wstring(*(double*)Data);
				case Type::String: return (*(std::wstring*)Data);
				default: return L"";
			}
		}
		inline bool ToBoolean() const {
			switch (tp) {
				case Type::Boolean: return (*(bool*)Data);
				case Type::Integer: return (*(int64_t*)Data);
				case Type::Float: return (*(double*)Data);
				case Type::String: return (*(std::wstring*)Data)==L"true";
				default: return 0;
			}
		}

		inline Variant operator+(const Variant& v) {
			switch (tp) {
				case Type::Boolean:
					switch (v.tp) {
						case Type::Boolean: return Variant(ToInteger() + v.ToInteger());
						case Type::Integer: return Variant(Get<int64_t>() + v.Get<int64_t>());
						case Type::Float: return Variant(ToInteger() + v.Get<double>());
						case Type::String: return Variant(ToString() + v.Get<std::wstring>());
						default: return nullptr;
					}
				case Type::Integer:
					switch (v.tp) {
						case Type::Boolean:
						case Type::Integer: return Variant(Get<int64_t>() + v.Get<int64_t>());
						case Type::Float: return Variant(Get<int64_t>() + v.Get<double>());
						case Type::String: return Variant(ToString() + v.Get<std::wstring>());
						default: return nullptr;
					}
				case Type::Float:
					switch (v.tp) {
						case Type::Boolean: return Variant(ToFloat() + v.ToInteger());
						case Type::Integer: return Variant(ToFloat() + v.ToInteger());
						case Type::Float: return Variant(ToFloat() + v.ToFloat());
						case Type::String: return Variant(Get<std::wstring>() + v.ToString());
						default: return nullptr;
					}
				case Type::String:
					switch (v.tp) {
						case Type::Boolean:
						case Type::Integer:
						case Type::Float: return Variant(ToString() + v.Get<std::wstring>());
						case Type::String: return Variant(Get<std::wstring>() + v.Get<std::wstring>());
						default: return nullptr;
					}
				default:return nullptr;
			}
		}
		inline Variant operator-(const Variant& v) {
			switch (tp) {
				case Type::Boolean:
					switch (v.tp) {
						case Type::Boolean:
						case Type::Integer: return Variant(ToInteger() - v.ToInteger());
						case Type::Float: return Variant(ToInteger() - v.ToFloat());
						default: return nullptr;
					}
				case Type::Integer:
					switch (v.tp) {
						case Type::Boolean:
						case Type::Integer: return Variant(ToInteger() - v.ToInteger());
						case Type::Float: return Variant(ToInteger() - v.ToFloat());
						default: return nullptr;
					}
				case Type::Float:
					switch (v.tp) {
						case Type::Boolean: return Variant(ToFloat() - v.ToInteger());
						case Type::Integer: return Variant(ToFloat() - v.ToInteger());
						case Type::Float: return Variant(ToFloat() - v.ToFloat());
						default: return nullptr;
					}
				default:return nullptr;
			}
		}
		inline Variant operator*(const Variant& v) {
			switch (tp) {
				case Type::Boolean:
					switch (v.tp) {
						case Type::Boolean: return Variant(ToBoolean() && v.ToBoolean());
						case Type::Integer: return Variant(ToInteger() * v.ToInteger());
						case Type::Float: return Variant(ToInteger() * v.ToFloat());
						case Type::String: return ToInteger() ? v.ToString() : L"";
						default: return nullptr;
					}
				case Type::Integer:
					switch (v.tp) {
						case Type::Boolean:
						case Type::Integer: return Variant(ToInteger() * v.ToInteger());
						case Type::Float: return Variant(ToInteger() * v.ToFloat());
						case Type::String: {
							std::wstringstream r;
							auto s = v.ToString();
							for (auto i = ToInteger(); i>0; i--) r << s;
							return r.str();
						}
						default: return nullptr;
					}
				case Type::Float:
					switch (v.tp) {
						case Type::Boolean: return Variant(ToFloat() * v.ToInteger());
						case Type::Integer: return Variant(ToFloat() * v.ToInteger());
						case Type::Float: return Variant(ToFloat() * v.ToFloat());
						case Type::String: {
							std::wstringstream r;
							auto s = v.ToString(); 
							auto i = ToFloat();
							for (; i>1.0; i--) r << s;
							r << s.substr(0, i * s.length());
							return r.str();
						}
						default: return nullptr;
					}
				case Type::String:
					switch (v.tp) {
						case Type::Boolean: return v.ToBoolean() ? ToString() : L"";
						case Type::Integer:{
							std::wstringstream r;
							auto s = ToString();
							for (auto i = v.ToInteger(); i>0; i--) r << s;
							return r.str();
						}
						case Type::Float: {
							std::wstringstream r;
							auto s = ToString();
							auto i = v.ToFloat();
							for (; i>1.0; i--) r << s;
							r << s.substr(0, i * s.length());
							return r.str();
						}
						case Type::String: return Variant(L"Am I a joke to you?");
						default: return nullptr;
					}
				default:return nullptr;
			}
		}
		inline Variant operator/(const Variant& v) {
			switch (tp) {
				case Type::Boolean:
					switch (v.tp) {
						case Type::Integer: return Variant(ToInteger() / v.ToInteger());
						case Type::Float: return Variant(ToInteger() / v.ToFloat());
						default: return Variant(ToFloat() / v.ToFloat());
					}
				case Type::Integer:
					switch (v.tp) {
						case Type::Integer: return Variant(ToInteger() / v.ToInteger());
						case Type::Float: return Variant(ToInteger() / v.ToFloat());
						default: return Variant(ToFloat() / v.ToFloat());
					}
				case Type::Float:
					switch (v.tp) {
						case Type::Integer: return Variant(ToFloat() / v.ToInteger());
						case Type::Float: return Variant(ToFloat() / v.ToFloat());
						default: return Variant(ToFloat() / v.ToFloat());
					}
				default:return Variant(ToFloat() / v.ToFloat());
			}
		}
		inline Variant operator%(const Variant& v) {
			return Variant(ToInteger() % v.ToInteger());
		}
	};

	class Code {
	public:
		enum class Type {
			Discard,

			Boolean,
			Integer,
			Float,
			String,
			Object,
			Array,
			Variable,

			Program,
			Block,
			CommaBlock,

			Index,
			Call,
			FunctionDecl,
			If,
			Else,
			While,
			For,
			Dot,
			Return,
			Break,
			Continue,
			Declare,

			MakeArray,
			MakeObject,

			IfThenElse,

			Assign,

			rPlusPlus,
			rMinusMinus,
			lPlusPlus,
			lMinusMinus,

			gt,
			gte,
			lt,
			lte,
			eq,
			neq,

			And,
			Or,
			Not,

			Add,
			Sub,
			Mul,
			Div,
			Mod,
			lShift,
			rShift,
			BitAnd,
			BitOr,
			BitNot,
			BitXor,

			UnaryMinus,
			UnaryPlus,

			AddAssign,
			SubAssign,
			MulAssign,
			DivAssign,
			ModAssign,
			lShiftAssign,
			rShiftAssign,
			BitAndAssign,
			BitOrAssign,
			BitNotAssign,
			BitXorAssign,
		};
		Type ty;
		Variant Value;
		std::vector<Code> Children;
	};

	Code Compile(std::wstring code);
}