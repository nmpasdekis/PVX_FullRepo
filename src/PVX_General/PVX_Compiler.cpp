#include <PVX_Compiler.h>
#include <array>
#include <tuple>
#include <PVX_Regex.h>
#include <list>

namespace PVX::Script {
	constexpr auto rOpts = std::regex_constants::ECMAScript | std::regex_constants::optimize;
	const std::wregex Escapes(LR"regex(\\.)regex", rOpts);
	const std::wregex spaces(LR"regex([\r\n\s\t]+)regex", rOpts);

	const std::array<std::tuple<std::wregex, std::wstring>, 11> match1{
		std::tuple{ std::wregex{ LR"regex(([^0-9a-zA-Z_]|^|\b)(let)([^0-9a-zA-Z_]|$))regex", rOpts }, L"\u0010" },
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)(for)([^0-9a-zA-Z_]|$))regex", rOpts), L"\u0011" },
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)(while)([^0-9a-zA-Z_]|$))regex", rOpts), L"\u0012" },
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)(if)([^0-9a-zA-Z_]|$))regex", rOpts), L"\u0013" },
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)(else)([^0-9a-zA-Z_]|$))regex", rOpts), L"\u0014" },
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)(function)([^0-9a-zA-Z_]|$))regex", rOpts), L"\u0015" },
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)(true|false)([^0-9a-zA-Z_]|$))regex", rOpts), L"\u0001" },
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)(return)([^0-9a-zA-Z_]|$))regex", rOpts), L"\u0002"},
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)([a-zA-Z_][0-9a-zA-Z_]*)([^0-9a-zA-Z_]|$))regex", rOpts), L"v" },
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)(\d*\.\d+)([^0-9a-zA-Z_]|$))regex", rOpts), L"\u0003" },
		std::tuple{ std::wregex( LR"regex(([^0-9a-zA-Z_]|^|\b)(\d+)([^0-9a-zA-Z_]|$))regex", rOpts), L"\u0004" }
	};
	class ExpressionMatch {
	public:
		std::wregex regex;
		std::wstring replace;
		int opIndex;
		std::wstring opText;
		std::vector<int> Indices;
		bool rename;
		ExpressionMatch(const wchar_t* pattern, const wchar_t* replace, int opIndex, const std::initializer_list<int>& Indices = {}, bool rename = false) :
			regex{ L"\\s*" + std::wstring{ pattern } + L"\\s*", rOpts }, replace{ replace }, opIndex{ opIndex }, opText{}, Indices{ Indices }, rename{ rename }{}

		ExpressionMatch(const wchar_t* pattern, const wchar_t* replace, const wchar_t* opText, const std::initializer_list<int>& Indices = {}, bool rename = false) :
			regex{ L"\\s*" + std::wstring{ pattern } + L"\\s*", rOpts }, replace{ replace }, opIndex{ -1 }, opText{ opText }, Indices{ Indices }, rename{ rename }{}
	};

	const std::array<ExpressionMatch, 2> matches11{
		ExpressionMatch{ LR"regex(([\u0001\u0003\u0004\"]))regex", L"r", 1, { 1 } },
		ExpressionMatch{ LR"regex(([v]))regex", L"l", 1, { 1 } }
	};
	//const std::array<ExpressionMatch, 41> matches2{
	const std::vector<ExpressionMatch> matches2 {
		//ExpressionMatch{ LR"regex(([rl])\s*([\/\*\%])\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*([\-\+])\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*(\<\<|\>\>)\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*(\<\=\>)\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*(\<\=|\>\=|\<|\>)\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*(\=\=|\!\=)\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*(\&)\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*(\^)\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*(\|)\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*(\&\&)\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*(\|\|)\s*([rl]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([lr])\s*\.\s*([l]))regex", L"l", L"->", { 1, 2 } },
		//ExpressionMatch{ LR"regex(([lr])(A))regex", L"l", L"Index", { 1, 2 } },
		//ExpressionMatch{ LR"regex(\[([rl])\])regex", L"A", L"A", { 1 } },
		//ExpressionMatch{ LR"regex((l)\s*(\<\<\=|\>\>\=|\+\=|\-\=|\*\=|\/\=|\%\=|\&\=|\^\=|\|\=|\=)\s*([rA]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex((l)\s*(\<\<\=|\>\>\=|\+\=|\-\=|\*\=|\/\=|\%\=|\&\=|\^\=|\|\=|\=)\s*([l]))regex", L"r", 2, { 1, 3 } },
		//ExpressionMatch{ LR"regex(([lr])\s*\:\s*(r))regex", L"p", L":", { 1, 2 } },
		//ExpressionMatch{ LR"regex(([lr])\s*\:\s*(l))regex", L"p", L":", { 1, 2 } },
		//ExpressionMatch{ LR"regex(\{\s*((p)\s*\,\s*)*(p)\s*\})regex", L"r", L"Object", { 2, 3 } },
		//ExpressionMatch{ LR"regex(([rl])\s*[,]\s*([lr]))regex", L"r", L",", { 1, 2 } },
		//ExpressionMatch{ LR"regex([\u0010]\s*(e))regex", L"e", L"let", { 1 } },
		ExpressionMatch{ LR"regex(\{\s*(e|;)*\s*\})regex", L"B", L"Block", { 1 } },
		//ExpressionMatch{ LR"regex(([l])\+\+)regex", L"r", L"++r",  { 1 } },
		//ExpressionMatch{ LR"regex(([l])\-\-)regex", L"r", L"--r",  { 1 } },
		//ExpressionMatch{ LR"regex(\+\+([l]))regex", L"r", L"++l",  { 1 } },
		//ExpressionMatch{ LR"regex(\-\-([l]))regex", L"r", L"--l",  { 1 } },
		//ExpressionMatch{ LR"regex(\u0002\s*(e))regex", L"e", L"Return",  { 1 } },
		//ExpressionMatch{ LR"regex(\u0015\s*([l])\s*\(\s*([rl]?)\s*\))regex", L"F", L"G", { 1, 2 }, true },
		//ExpressionMatch{ LR"regex(([F])\s*(B))regex", L"r", L"FunctionDecl", { 1, 2 } },
		//ExpressionMatch{ LR"regex(([lr])\s*\(\s*([rl]?)\s*\))regex", L"l", L"FunctionCall", { 1, 2 } },
		//ExpressionMatch{ LR"regex([\u0011]\s*\(\s*(e|;)\s*(e|;)\s*([lr]?)\s*\)\s*(B|e|;))regex", L"e", L"for", { 1, 2, 3, 4 } },
		//ExpressionMatch{ LR"regex([\u0012]\s*\(\s*([lr])\s*\)\s*(B|e|;))regex", L"e", L"while", { 1, 2 } },
		//ExpressionMatch{ LR"regex([\u0013]\s*\(\s*([lr])\s*\)\s*(B|e|;)\s*[\u0014]\s*(B|e|;))regex", L"e", L"if", { 1, 2, 3 } },
		//ExpressionMatch{ LR"regex([\u0013]\s*\(\s*([lr])\s*\)\s*(B|e|;))regex", L"e", L"if", { 1, 2 } },
		//ExpressionMatch{ LR"regex(!([lr]))regex", L"r", L"!",  { 1 } },
		//ExpressionMatch{ LR"regex(\(\s*(r)\s*\))regex", L"r", 1, { 1 }, true },
		//ExpressionMatch{ LR"regex(\(\s*(l)\s*\))regex", L"l", 1, { 1 }, true },
		//ExpressionMatch{ LR"regex(\-([lr]))regex", L"r", L"-u",  { 1 } },
		//ExpressionMatch{ LR"regex(\+([lr]))regex", L"r", L"+u",  { 1 } },
		//ExpressionMatch{ LR"regex(([lr])\s*\;)regex", L"e", L"e", { 1 }, true },
		//ExpressionMatch{ LR"regex(([rl]))regex", L"e", 1, { 1 }, true },
	};


	std::wstring RemoveStrings(const std::wstring& text, std::vector<std::wstring>& Strings) {
		using namespace std::string_literals;
		std::wstring txt = text;
		std::wstringstream ret;
		while (true) {
			int index = txt.find(L'"');
			if (index == -1) { ret << txt; break; }
			ret << (txt.substr(0, index) + L"\"");
			txt = txt.substr(index + 1);
			index = txt.find(L'"');
			if (index == -1) throw ("parse Error: mismatched quotes");
			while (true) {
				if (index < txt.size() - 1 && txt[index + 1] == L'"') index++;
				if (index == 0 || (txt[index - 1] != L'"' && txt[index - 1] != '\\')) break;
				index = txt.find(L'"', index + 1);
				if (index == -1) throw ("parse Error: mismatched quotes");
			}

			

			Strings.push_back(PVX::Replace(txt.substr(0, index), Escapes, [](const std::wsmatch& m) -> std::wstring {
				auto test = m.str();
				switch (test[1]) {
					case L'n': return L"\n";
					case L'r': return L"\r";
					case L't': return L"\t";
				}
				return m.str();
			}));

			txt = txt.substr(index + 1);
		}
		return ret.str();
	}
	const std::wregex RemoveLineComments(LR"regex(//.*)regex", rOpts);
	const std::wregex RemoveInlineCommentStart(LR"regex(/\*)regex", rOpts);
	const std::wregex RemoveInlineCommentEnd(LR"regex(\*/)regex", rOpts);
	std::wstring RemoveComments(const std::wstring& Code) {

		auto ret = std::regex_replace(Code, RemoveLineComments, L"");
		//ret = std::regex_replace(ret, RemoveInLineComments, L"");

		while (true) {
			int start = PVX::regex_search(RemoveInlineCommentStart, ret);
			if (-1 == start) break;
			int end = PVX::regex_search(RemoveInlineCommentEnd, ret);
			if (-1 == end) end = ret.size() - 2;
			ret = ret.replace(start, end - start + 2, L"");
		}
		return ret;
	}

	struct node {
		std::wstring Text;
		std::vector<node> Children;
	};

	inline void Replace(std::vector<node>& vec, int from, int size, const node&& n) {
		vec[from] = n;
		if (size > 1) {
			auto c2 = from + 1;
			for (auto c = from + size; c < vec.size(); c++) vec[c2++] = std::move(vec[c]);
			vec.resize(vec.size()+1-size);
		}
	}
	inline std::vector<node> Sublist(std::vector<node>& list, int from, int size) {
		std::vector<node> ret;
		ret.reserve(size);
		for (auto i = 0; i<size; i++) {
			ret.push_back(std::move(list[from+i]));
		}
		return ret;
	}

	std::vector<std::vector<std::wstring>> findEx(const std::wstring& s, const std::wregex rx) {
		std::vector<std::vector<std::wstring>> captured_groups;
		std::vector<std::wstring> captured_subgroups;
		const std::wsregex_token_iterator end_i;
		for (std::wsregex_token_iterator i(s.cbegin(), s.cend(), rx); i != end_i; ++i) {
			captured_subgroups.clear();
			std::wstring group = *i;
			std::wsmatch res;
			if (regex_search(group, res, rx)) {
				for (unsigned i = 0; i<res.size(); i++)
					captured_subgroups.push_back(res[i]);

				if (captured_subgroups.size() > 0)
					captured_groups.push_back(captured_subgroups);
			}

		}
		return captured_groups;
	}
	inline bool Detect(std::wstring& Code, std::vector<node>& tokens, const ExpressionMatch& mm) {
		std::wsmatch m2;
		bool result = false;

		std::vector<node> newChildren;

		//auto test = findEx(Code, mm.regex);

		while (std::regex_search(Code, m2, mm.regex) && !m2.empty()) {
			for (auto idx: mm.Indices) {
				std::wsmatch sm = m2[idx];
				//auto [a, b] = sm;
			}
		}

		return result;
	}

	Code Compile(std::wstring code) {

		std::vector<node> tokens = [&]{
			std::vector<std::wstring> Strings;
			code = RemoveComments(code);
			code = RemoveStrings(code, Strings);
			code = std::regex_replace(code, spaces, L" ");
			std::vector<node> ret(code.size());
			int j = 0;
			for (auto i = 0; i<code.size(); i++) if (code[i]==L'"')
				ret[i].Text = Strings[j++];
			return ret;
		}();

		for (auto& m : match1) {
			code = PVX::Replace(code, std::get<0>(m), [&tokens, &m](const std::wsmatch& m2) -> std::wstring {
				auto str = m2[2].str();
				Replace(tokens, m2.position() + m2[2].first._Ptr - m2[0].first._Ptr, str.size(), node{ str });
				return m2[1].str() + std::get<1>(m) + m2[3].str();
			});
		}
		//for (auto& mm : matches2) Detect(code, tokens, mm);

		code = L"{eee}{eee}";

		bool more = true;
		while (more) {
			more = false;
			for (auto& mm : matches2) {
				if (more = Detect(code, tokens, mm))
					break;
			}
		}

		return Code{};
	}
}