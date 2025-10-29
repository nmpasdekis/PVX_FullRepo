#include <PVX_Lua.h>
#include<PVX_String.h>

namespace PVX::Scripting{
	size_t findFormat(const std::string_view & txt, size_t start) {
		constexpr auto end = std::string::npos;
		char last = 0;
		bool inString = false;
		std::vector<std::string> formats;
		std::vector<char> stack;
		for (auto cur = start; cur < txt.size(); cur++) {
			auto cc = txt[cur];
			switch (cc) {
			case '-':
				if (!inString && last == '-') {
					if (cur + 2 < txt.size() && txt[cur + 1] == '[' && txt[cur + 2] == '[') {
						auto s = txt.find("]]--", cur);
						if (s == end) throw "Did not find ]]--";
						cur = s + 3;
					} else {
						auto s = txt.find("\n", cur);
						if (s == end) s = txt.size() - 1;
						cur = s;
					}
				}
				break;
			case '[':
				if (!inString && txt[cur + 1] == '[') {
					auto s = txt.find("]]", cur + 1);
					if (s == end) throw "Did not find ]]";
					cur = s + 1;
				}
				break;
			case '"': case '\'':
				if (!inString) {
					stack.push_back(cc);
					inString = true;
				} else {
					if (stack.back() == cc && last != '\\') {
						inString = false;
					}
				}
				break;
			case '`':
				if (!inString) return cur;
				break;
			}
			last = txt[cur];
		}
		return end;
	}

	std::string pp(const std::string & txt) {


		const char * c_str = txt.c_str();




		std::vector<char> stack;
		constexpr auto end = std::string::npos;
		size_t start = 0;
		auto index = findFormat(txt, start);
		if (index == end) return txt;
		stack.push_back('`');

		std::stringstream ret;
		ret << txt.substr(start, index - start);
		start = index + 1;

		std::string tmp;
		std::vector<std::string> formats;

		while (true) {
			auto dolar = txt.find("${", start);
			auto endQuote = txt.find('`', start);

			if (endQuote < dolar) {
				ret << "[[" << txt.substr(start, endQuote - start) << "]]";
				stack.pop_back();
				start = endQuote + 1;
			} else {
				if (dolar == end) throw "mismatched '`'";
				stack.push_back('$');
				ret << "string.format([[" << PVX::Replace(txt.substr(start, dolar - start), "%", "%%") << "%s";

				start = index = dolar + 2;

				while (stack.size() && index < txt.size()) {
					if (txt[index] == '\\') { index += 2; continue; }
					if (txt[index] == '`') {
						if (stack.back() == '`') {
							if (stack.size() == 1) {
								tmp = PVX::Replace(txt.substr(start, index - start), "%", "%%");
								start = index + 1;
								ret << tmp;
							}
							stack.pop_back();
						} else 
							stack.push_back('`');
					} else if (txt[index] == '$' && index < txt.size() - 1 && txt[index + 1] == '{') {
						if (stack.size() == 1) {
							tmp = PVX::Replace(txt.substr(start, index - start), "%", "%%");
							start = index + 2;
							ret << tmp << "%s";
						}
						stack.push_back('$');
						index++;
					} else if (txt[index] == '{') {
						stack.push_back('{');
					} else if (txt[index] == '}') {
						if (stack.back() == '{' || (stack.back() == '$' && stack.size()>2))
							stack.pop_back();
						else if (stack.back() == '$') {
							formats.push_back(txt.substr(start, index - start));
							start = index + 1;
							stack.pop_back();
						} 
						else throw "unexpected '}'";
					}
					index++;
				}
				if (stack.size()) throw "Error";
				ret << "]], ";
				
				if(formats.size())
					ret << "(" << pp(formats[0]) << ")";
				for (auto i = 1; i < formats.size(); i++) {
					ret << ", (" << pp(formats[i]) << ")";
				}
				ret << ")";
				start = index;
			}

			index = findFormat(txt, start);
			if (index != end) {
				stack.push_back('`');
				ret << txt.substr(start, index - start);
			} else {
				ret << txt.substr(start, txt.size() - start);
				return ret.str();
			}
		}
	}

	std::string preprocess(std::string code) {
		return pp(code);
	}

	myContainer<luaCppFunction> myFunctions;
}