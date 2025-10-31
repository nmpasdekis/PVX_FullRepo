#include <iostream>
#include <PVX_Tokenizer.h>

int main() {
	PVX::PVX_CppTokenizer tokenizer(R"code(

#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>


/*
test Munti-line comment
*/

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
		unsigned int Test;
		State state = State::None;
		std::vector<std::string> operators;
		std::unordered_map<char, std::unordered_map<char, std::unordered_map<char, bool>>> opNext3;
		std::unordered_map<char, std::unordered_map<char, bool>> opNext2;
		std::unordered_map<char, bool> opNext1;
		std::string buffer;
		std::string_view code;
		size_t cur = 0;
		inline void initOps() {
			std::sort(operators.begin(), operators.end(), [](const std::string& a, const std::string& b) {
				return a.size() > b.size() || (a.size() == b.size() && a < b);
			});
			for (const auto& op : operators) {
				if (op.size() == 3) {
					opNext3[op[0]][op[1]][op[2]] = true;
				}
				else if (op.size() == 2) {
					opNext2[op[0]][op[1]] = true;
				}
				else opNext1[op[0]] = true;
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
			return opNext3.contains(c) || opNext2.contains(c) || opNext1.contains(c);
		}
		inline std::string getOperator() {
			if (more(2) && opNext3.contains(code[cur]) && opNext3.at(code[cur]).contains(code[cur + 1]) && opNext3.at(code[cur]).at(code[cur + 1]).contains(code[cur + 2])) {
				cur += 3;
				return std::string{ code.substr(cur - 3, 3) };
			}
			if (more() && opNext2.contains(code[cur]) && opNext2.at(code[cur]).contains(code[cur + 1])) {
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
	public:
		PVX_Tokenizer(const char * code) : code{ code, std::strlen(code) }, operators { { "+", "-", "*", "/", "%", "++", "--", "=", "+=", "-=", "*=", "/=", "%=", "&", "|", "^", "~", "&=", "|=", "^=", "<<", ">>", "<<=", ">>=", "!", "!=", "&&", "||", "==", "<", ">", "<=", ">=", "<=>", ".", "->", ".*", "->*", "::", "?", ":", ",", ";", "...", "(", ")", "[", "]", "{", "}", "#", "##" } } {
			initOps();
		}
		PVX_Tokenizer(const char * code, std::vector<std::string> operators) : code{ code, std::strlen(code) }, operators{ operators } {
			initOps();
		}

		std::string GetToken() {
			if (!skipSpaces()) return "";
			if (code[cur] == '.' && more() && isDigit(code[cur + 1])) return getNumber();
			if (isOperator(code[cur])) return getOperator();
			if (isDigit(code[cur])) return getNumber();
			if (isAlpha(code[cur])) {
				auto id = getIdentifier();
				if (!end() && code[cur] == '"') {
					if (id.back() != 'R')
						return getRawString();
					return id + getString();
				}
				return id;
			}
			if (code[cur] == '"') return getString();
			return "";
		}
	};
}

)code");
	auto tokens = tokenizer.Tokenize();

	return 0;
}