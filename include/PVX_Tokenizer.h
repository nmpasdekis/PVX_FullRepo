#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>


namespace PVX {
	class PVX_Tokenizer {
		enum class State {
			None,
			Identifier,
			Integer,
			Float,
			String,
			Char,
			Operator,
			Comment_SingleLine,
			Comment_MultiLine
		};
		State state = State::None;
		std::vector<std::string> operators;
		std::unordered_map<char, std::unordered_map<char, std::unordered_set<char>>> opNext3;
		std::unordered_map<char, std::unordered_set<char>> opNext2;
		std::unordered_set<char> opNext1;
		std::string buffer;
		std::string_view code;
		size_t cur = 0, last = 0;
		inline void initOps() {
			std::sort(operators.begin(), operators.end(), [](const std::string& a, const std::string& b) {
				return a.size() > b.size() || (a.size() == b.size() && a < b);
			});
			for (const auto& op : operators) {
				if (op.size() == 3) {
					opNext3[op[0]][op[1]].insert(op[2]);
				}
				else if (op.size() == 2) {
					opNext2[op[0]].insert(op[1]);
				}
				else opNext1.insert(op[0]);
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
				return std::string{ code.substr(cur - 3, 3) };
			}
			if (more() && opNext2.count(code[cur]) && opNext2.at(code[cur]).count(code[cur + 1])) {
				cur += 2;
				return std::string{ code.substr(cur - 2, 2) };
			}
			cur += 1;
			return std::string{ code.substr(cur - 1, 1) };
		}
		inline bool skipSpaces() {
			while (cur < code.size() && isSpace(code[cur])) cur++;
			return cur < code.size();
		}
		inline std::string getNumber() {
			size_t start = cur;
			if (code[cur] == '.') cur++;
			while (cur < code.size() && isDigit(code[cur])) cur++;
			if (cur < code.size() && code[cur] == '.') {
				cur++;
				while (cur < code.size() && isDigit(code[cur])) cur++;
				if (cur < code.size() && (code[cur] == 'e' || code[cur] == 'E')) {
					cur++;
					if (cur < code.size() && (code[cur] == '+' || code[cur] == '-')) cur++;
					while (cur < code.size() && isDigit(code[cur])) cur++;
				}
				else if (cur < code.size() && (isAlpha(code[cur]))) {
					while (isAlphaNumeric(code[cur])) cur++;
				}
			}
			return std::string{ code.substr(start, cur - start) };
		}
		inline std::string getIdentifier() {
			size_t start = cur;
			while (cur < code.size() && isAlphaNumeric(code[cur])) cur++;
			return std::string{ code.substr(start, cur - start) };
		}
		inline std::string getString() {
			char quote = code[cur];
			size_t start = cur;
			cur++;
			while (cur < code.size()) {
				if (code[cur] == '\\' && more()) {
					cur += 2;
				}
				else if (code[cur] == quote) {
					cur++;
					break;
				}
				else {
					cur++;
				}
			}
			if (!end() && isAlpha(code[cur])) {
				while (isAlphaNumeric(code[cur])) cur++;
			}
			return std::string{ code.substr(start, cur - start) };
		}
		inline std::string getRawString() {
			using namespace std::string_literals;
			size_t start = cur;
			cur++;
			auto paren = code.find('(', cur);
			auto raw_end = ")"s + std::string(code.substr(cur, paren - cur)) + "\""s;
			auto end_pos = code.find(raw_end, paren);
			return std::string{ code.substr(start, end_pos + raw_end.size() - start) };
		}
		inline void SkipSingleLineComment() {
			cur += 2;
			while (cur < code.size() && code[cur] != '\n') cur++;
		}
		inline void SkipMultiLineComment() {
			cur += 2;
			while (cur + 1 < code.size()) {
				if (code[cur] == '*' && code[cur + 1] == '/') { cur += 2; break; }
				cur++;
			}
		}
		inline void SetPosition(size_t position) {
			cur = position;
		}
		inline size_t GetPosition() {
			return cur;
		}
		friend class PVX_CppTokenizer;
	public:
		struct Token {
			std::string Value;
			enum class Type {
				Unknown,
				End,
				Identifier,
				Operator,
				Number,
				String,
			} TokenType;
		};
		PVX_Tokenizer(const char * code) : code{ code, std::strlen(code) }, operators { { "+", "-", "*", "/", "%", "++", "--", "=", "+=", "-=", "*=", "/=", "%=", "&", "|", "^", "~", "&=", "|=", "^=", "<<", ">>", "<<=", ">>=", "!", "!=", "&&", "||", "==", "<", ">", "<=", ">=", "<=>", ".", "->", ".*", "->*", "::", "?", ":", ",", ";", "...", "(", ")", "[", "]", "{", "}", "#", "##" } } {
			initOps();
		}
		PVX_Tokenizer(const char * code, std::vector<std::string> operators) : code{ code, std::strlen(code) }, operators{ operators } {
			initOps();
		}

		Token GetTokenWithSemantics() {
			if (!skipSpaces()) return { "", Token::Type::End };
			if (code[cur] == '/' && more() && code[cur + 1] == '/') {
				SkipSingleLineComment();
				if (!skipSpaces()) return { "", Token::Type::End };
			}
			if (code[cur] == '/' && more() && code[cur + 1] == '*') {
				SkipMultiLineComment();
				if (!skipSpaces()) return { "", Token::Type::End };
			}
			if (code[cur] == '.' && more() && isDigit(code[cur + 1])) return { getNumber(),Token::Type::Number };
			if (isOperator(code[cur])) return { getOperator(), Token::Type::Operator };
			if (isDigit(code[cur])) return { getNumber(), Token::Type::Number };
			if (isAlpha(code[cur])) {
				auto id = getIdentifier();
				if (!end() && (code[cur] == '"')) {
					if (id.back() != 'R')
						return { id + getRawString(), Token::Type::String };
					return { id + getString(), Token::Type::String };
				}
				return { id, Token::Type::Identifier };
			}
			if (code[cur] == '"' || code[cur] == '\'') return { getString(), Token::Type::String };
			return { "", Token::Type::Unknown };
		}
		std::string GetToken() {
			return GetTokenWithSemantics().Value;
		}
		std::vector<std::string> Tokenize() {
			std::vector<std::string> tokens;
			Token token;
			while ((token = GetTokenWithSemantics()).TokenType > Token::Type::End) {
				tokens.push_back(token.Value);
			}
			tokens.shrink_to_fit();
			return tokens;
		}
		std::vector<Token> TokenizeWithSemanticse() {
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



	class PVX_CppTokenizer {
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
				Separator
			} TokenType;
		};
	protected:
		PVX_Tokenizer tokenizer;
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
			case PVX_Tokenizer::Token::Type::Identifier:
				if (keywords.count(token.Value)) return { token.Value, CppToken::Type::Keyword };
				if (types.count(token.Value)) return { token.Value, CppToken::Type::Type };
				cppToken.TokenType = CppToken::Type::Identifier;
				break;
			case PVX_Tokenizer::Token::Type::Operator:
				cppToken.TokenType = CppToken::Type::Operator;
				break;
			case PVX_Tokenizer::Token::Type::Number:
				cppToken.TokenType = CppToken::Type::NumberLiteral;
				break;
			case PVX_Tokenizer::Token::Type::String:
				cppToken.TokenType = CppToken::Type::StringLiteral;
				break;
			default:
				cppToken.TokenType = CppToken::Type::Unknown;
				break;
			}
			return cppToken;
		}
	public:
		inline PVX_CppTokenizer(const char* code) : tokenizer(code) {}
		std::vector<CppToken> Tokenize() {
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
	};
}