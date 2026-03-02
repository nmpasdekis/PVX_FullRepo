#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace PVX {
	class Tokenizer {
		enum class State {
			None,
			Identifier,
			Integer,
			Float,
			String,
			Char,
			Operator
		};
		std::vector<std::string> operators;
		std::unordered_map<char, std::unordered_map<char, std::unordered_set<char>>> opNext3;
		std::unordered_map<char, std::unordered_set<char>> opNext2;
		std::unordered_set<char> opNext1;
		std::string buffer;
		std::string_view code;
		size_t line = 1;
		size_t col = 1;
		size_t cur = 0, last = 0;
		inline void initOps() {
			std::sort(operators.begin(), operators.end(), [](const std::string& a, const std::string& b) {
				return a.size() > b.size() || (a.size() == b.size() && a < b);
			});
			for (const auto& op : operators) {
				if (op.size() == 3) {
					opNext3[op[0]][op[1]].insert(op[2]);
				} else if (op.size() == 2) {
					opNext2[op[0]].insert(op[1]);
				} else opNext1.insert(op[0]);
			}
		}
		inline bool end() {
			return cur >= code.size();
		}
		inline bool more(int sz = 1) {
			return cur + sz < code.size();
		}
		inline bool isSpace(char c) {
			return c == ' ' || c == '\t' || c == '\n' || c == '\r';
		}
		inline bool isAlpha(char c) {
			return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
		}
		inline bool isHex(char c) {
			return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || (c >= '0' && c <= '9') || c == '\'';
		}
		inline bool isBin(char c) {
			return c == '0' || c == '1' || c == '\'';
		}
		inline bool isOct(char c) {
			return c >= '0' && c <= '7' || c == '\'';
		}
		inline bool isDigit2(char c) {
			return c >= '0' && c <= '9' || c == '\'';
		}
		inline bool isDigit(char c) {
			return c >= '0' && c <= '9';
		}
		inline bool isAlphaNumeric(char c) {
			return isAlpha(c) || isDigit(c);
		}
		inline bool isOperator(char c) {
			return opNext3.count(c) || opNext2.count(c) || opNext1.count(c);
		}
		inline std::string getOperator() {
			if (more(2) && opNext3.count(code[cur]) && opNext3.at(code[cur]).count(code[cur + 1]) && opNext3.at(code[cur]).at(code[cur + 1]).count(code[cur + 2])) {
				cur += 3;
				col += 3;
				return std::string{ code.substr(cur - 3, 3) };
			}
			if (more() && opNext2.count(code[cur]) && opNext2.at(code[cur]).count(code[cur + 1])) {
				cur += 2;
				col += 2;
				return std::string{ code.substr(cur - 2, 2) };
			}
			cur += 1;
			col += 1;
			return std::string{ code.substr(cur - 1, 1) };
		}
		inline bool skipSpaces() {
			while (cur < code.size() && isSpace(code[cur])) {
				if (code[cur] == '\n'){
					line++;
					col = 0;
				}
				cur++; col++;
			}
			return cur < code.size();
		}
		inline std::string getNumber() {
			size_t start = cur;
			if (code[cur] == '0' && cur + 1 < code.size() && (code[cur + 1]=='x'|| code[cur + 1] == 'X')) {
				cur += 2; col += 2;
				while (cur < code.size() && isHex(code[cur])) { cur++; col++; }
			} else if (code[cur] == '0' && cur + 1 < code.size() && (code[cur + 1] == 'b' || code[cur + 1] == 'B')) {
				cur += 2; col += 2;
				while (cur < code.size() && isBin(code[cur])) { cur++; col++; }
			} else if (code[cur] == '0' && cur + 1 < code.size() && isDigit(code[cur + 1])) {
				cur += 2; col += 2;
				while (cur < code.size() && isOct(code[cur])) { cur++; col++; }
			} else {
				if (code[cur] == '.') cur++;
				while (cur < code.size() && isDigit2(code[cur])) { cur++; col++; }
				if (cur < code.size() && code[cur] == '.') {
					cur++; col++;
					while (cur < code.size() && isDigit2(code[cur])) { cur++; col++; }
				}
				if (cur < code.size() && (code[cur] == 'e' || code[cur] == 'E')) {
					cur++;
					if (cur < code.size() && (code[cur] == '+' || code[cur] == '-')) { cur++; col++; }
					while (cur < code.size() && isDigit(code[cur])) { cur++; col++; }
				}
			}
			if (cur < code.size() && (isAlpha(code[cur]))) {
				while (cur < code.size() && isAlphaNumeric(code[cur])) { cur++; col++; }
			}
			return std::string{ code.substr(start, cur - start) };
		}
		inline std::string getIdentifier() {
			size_t start = cur;
			while (cur < code.size() && isAlphaNumeric(code[cur])) { cur++; col++; }
			return std::string{ code.substr(start, cur - start) };
		}
		inline std::string getString() {
			char quote = code[cur];
			size_t start = cur;
			cur++;
			while (cur < code.size()) {
				if (code[cur] == '\\' && more()) {
					cur += 2; col += 2;
				}
				else if (code[cur] == quote) {
					cur++; col++;
					break;
				}
				else {
					cur++; col++;
				}
			}
			if (!end() && isAlpha(code[cur])) {
				while (isAlphaNumeric(code[cur])) { cur++; col++; }
			}
			return std::string{ code.substr(start, cur - start) };
		}
		inline std::string getRawString() {
			using namespace std::string_literals;
			size_t start = cur;
			cur++; col++;
			auto paren = code.find('(', cur);
			auto raw_end = ")"s + std::string(code.substr(cur, paren - cur)) + "\""s;
			auto end_pos = code.find(raw_end, paren);
			auto raw_size = end_pos + raw_end.size() - start;
			for (auto i = 1; i < raw_size; i++, cur++, col++) {
				if (code[cur] == '\n') {
					line++;
					col = 0;
				}
			}
			while (isAlphaNumeric(code[cur])) { cur++; col++; }
			return std::string{ code.substr(start, cur - start) };
		}
		inline void SkipSingleLineComment() {
			cur += 2;
			while (cur < code.size() && code[cur] != '\n') cur++;
			cur++;
			line++;
			col = 1;
		}
		inline void SkipMultiLineComment() {
			cur += 2;
			col += 2;
			while (cur + 1 < code.size()) {
				if (code[cur] == '*' && code[cur + 1] == '/') { cur += 2; col += 2; break; }
				if (code[cur] == '\n') { line++; col = 0; }
				cur++; col++;
			}
		}
		inline void SetPosition(size_t position) {
			cur = position;
		}
		inline size_t GetPosition() {
			return cur;
		}
		friend class CppTokenizer;
	public:
		struct Token {
			std::string Value = {};
			enum class Type {
				Unknown,
				End,
				Identifier,
				Operator,
				Number,
				String,

				Special1,
				Special2,
				Special3,
				Function,
				Function1
			} TokenType = Type::Unknown;
			size_t line;
			size_t column;
			char * Filename;
			static inline Token makeString(const std::string& str) { return Token{ .Value = str, .TokenType = Type::String }; }
			static inline Token makeNumber(int64_t num) { return Token{ .Value = std::to_string(num), .TokenType = Type::Number }; }
			static inline Token makeNumber(float num) { return Token{ .Value = std::to_string(num) + "f", .TokenType = Type::Number}; }
			static inline Token makeNumber(double num) { return Token{ .Value = std::to_string(num), .TokenType = Type::Number }; }
			static inline Token makeNumber(std::string num) { return Token{ .Value = std::to_string(std::stoll(num, nullptr, 0)), .TokenType = Type::Number }; }
		};
		Tokenizer(const char * code) : code{ code, std::strlen(code) }, operators { { "+", "-", "*", "/", "%", "++", "--", "=", "+=", "-=", "*=", "/=", "%=", "&", "|", "^", "~", "&=", "|=", "^=", "<<", ">>", "<<=", ">>=", "!", "!=", "&&", "||", "==", "<", ">", "<=", ">=", "<=>", ".", "->", ".*", "->*", "::", "?", ":", ",", ";", "...", "(", ")", "[", "]", "{", "}", "#", "##" }} {
			initOps();
		}
		Tokenizer(const char * code, std::vector<std::string> operators) : code{ code, std::strlen(code) }, operators{ operators } {
			initOps();
		}
		Tokenizer(std::vector<std::string> operators): operators{ operators } {
			initOps();
		}

		inline Tokenizer& reset(const char * code) {
			this->code = std::string_view{ code, std::strlen(code) };
			cur = 0; line = 1; col = 1;
			return *this;
		}
		inline Token GetTokenWithSemantics() {
			while (cur < code.size()) {
				if (!skipSpaces()) return { "", Token::Type::End, line, col };
				if (code[cur] == '/' && more() && code[cur + 1] == '/') {
					SkipSingleLineComment();
					continue;
				}
				if (code[cur] == '/' && more() && code[cur + 1] == '*') {
					SkipMultiLineComment();
					continue;
				}
				if (code[cur] == '.' && more() && isDigit(code[cur + 1])) return { getNumber(),Token::Type::Number };
				if (isOperator(code[cur])) {
					auto l = line;
					auto c = col;
					return { getOperator(), Token::Type::Operator, l, c };
				}
				if (isDigit(code[cur])) {
					auto l = line;
					auto c = col;
					return { getNumber(), Token::Type::Number, l, c };
				}
				if (isAlpha(code[cur])) {
					auto l = line;
					auto c = col;
					auto id = getIdentifier();
					if (!end() && (code[cur] == '"')) {
						if (id.back() == 'R')
							return { id + getRawString(), Token::Type::String, l, c };
						return { id + getString(), Token::Type::String, l, c };
					}
					return { id, Token::Type::Identifier, l, c };
				}
				if (code[cur] == '"' || code[cur] == '\'') {
					auto l = line;
					auto c = col;
					return { getString(), Token::Type::String, l, c };
				}
			}
			return { "", Token::Type::Unknown, line, col };
		}
		inline std::string GetToken() {
			return GetTokenWithSemantics().Value;
		}
		inline std::vector<std::string> Tokenize() {
			std::vector<std::string> tokens;
			Token token;
			while ((token = GetTokenWithSemantics()).TokenType > Token::Type::End) {
				tokens.push_back(token.Value);
			}
			tokens.shrink_to_fit();
			return tokens;
		}
		inline std::vector<Token> TokenizeWithSemanticse() {
			std::vector<Token> tokens;
			Token token;
			Token lastToken;
			while ((token = GetTokenWithSemantics()).TokenType > Token::Type::End) {
				tokens.push_back(token);
			}
			tokens.shrink_to_fit();
			return tokens;
		}
	};



	class CppTokenizer {
	public:
		struct CppToken {
			std::string Value;
			enum class Type {
				Unknown,
				End,
				Type,
				Identifier,
				Keyword,
				Operator,
				NumberLiteral,
				StringLiteral,
				Preprocessor
			} TokenType;
			CppToken* close = nullptr;
			size_t line;
			size_t column;
			char * Filename;
		};
	protected:
		Tokenizer tokenizer;
		std::unordered_set<std::string> keywords = {
			"alignas", "alignof", "and", "and_eq", "asm",
			"atomic_cancel", "atomic_commit", "atomic_noexcept", "auto",
			"bitand", "bitor", "break", "case", "catch",
			"class", "compl", "concept", "const", "consteval",
			"constexpr", "constinit", "const_cast", "continue",
			"co_await", "co_return", "co_yield", "decltype", "default",
			"delete", "do", "dynamic_cast", "else", "enum",
			"explicit", "export", "extern", "false", "for", "friend",
			"goto", "if", "inline", "mutable", "namespace", "new",
			"noexcept", "not", "not_eq", "nullptr", "operator", "or",
			"or_eq", "private", "protected", "public", "reflexpr",
			"register", "reinterpret_cast", "requires", "return",
			"sizeof", "static", "static_assert", "static_cast",
			"struct", "switch", "synchronized", "template", "this",
			"thread_local", "throw", "true", "try", "typedef",
			"typeid", "typename", "union", "using", "virtual",
			"volatile", "while", "xor", "xor_eq"
		};
		std::unordered_set<std::string> types = {
			"bool", "char", "char8_t", "char16_t", "char32_t", "double", "float", "int", "long", "short", "signed", "unsigned", "void", "wchar_t", "size_t", "ptrdiff_t", "nullptr_t", "max_align_t", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int_fast8_t", "int_fast16_t", "int_fast32_t", "int_fast64_t", "uint_fast8_t", "uint_fast16_t", "uint_fast32_t", "uint_fast64_t", "int_least8_t", "int_least16_t", "int_least32_t", "int_least64_t", "uint_least8_t", "uint_least16_t", "uint_least32_t", "uint_least64_t", "intmax_t", "uintmax_t", "intptr_t", "uintptr_t", "byte"
		};

		CppToken GetToken() {
			auto token = tokenizer.GetTokenWithSemantics();
			CppToken cppToken;
			cppToken.Value = token.Value;
			switch (token.TokenType) {
			case Tokenizer::Token::Type::Identifier:
				if (keywords.count(token.Value)) return { token.Value, CppToken::Type::Keyword, 0 };
				if (types.count(token.Value)) return { token.Value, CppToken::Type::Type, 0 };
				cppToken.TokenType = CppToken::Type::Identifier;
				break;
			case Tokenizer::Token::Type::Operator:
				if (cppToken.Value != "#") {
					cppToken.TokenType = CppToken::Type::Operator;
				} else {
					cppToken.Value += tokenizer.GetToken();
					cppToken.TokenType = CppToken::Type::Preprocessor;
				}
				break;
			case Tokenizer::Token::Type::Number:
				cppToken.TokenType = CppToken::Type::NumberLiteral;
				break;
			case Tokenizer::Token::Type::String:
				cppToken.TokenType = CppToken::Type::StringLiteral;
				break;
			default:
				cppToken.TokenType = CppToken::Type::Unknown;
				break;
			}
			return cppToken;
		}
	public:
		inline CppTokenizer(const char* code) : tokenizer(code) {}
		inline std::vector<CppToken> Tokenize() {
			std::vector<CppToken> tokens;
			CppToken token;
			
			while ((token = GetToken()).TokenType > CppToken::Type::End) {
				if (token.TokenType == CppToken::Type::Type) {
					auto start = tokenizer.GetPosition();
					std::stringstream combined;
					combined << token.Value;
					auto nextToken = GetToken();
					while (nextToken.TokenType == CppToken::Type::Type) {
						start = tokenizer.GetPosition();
						combined << " " << nextToken.Value;
						nextToken = GetToken();
					}
					tokenizer.SetPosition(start);
					token.Value = combined.str();
				}
				tokens.push_back(token);

			}
			tokens.shrink_to_fit();
			return tokens;
		}
		inline CppTokenizer& reset(const char* code) {
			tokenizer.reset(code);
			return *this;
		}
	};
}