#ifndef __PVX_STD_STRING_H__
#define __PVX_STD_STRING_H__
#include <string>
#include <vector>
#include<functional>
#include <sstream>
#include <regex>
#include <string>
#include <array>
#include <PVX_Regex.h>

namespace PVX {
	namespace String {
		inline void OnSplit(const std::string& Text, const std::string& Separator, std::function<void(const std::string&)> clb) {
			size_t ssz = Separator.size(), last = 0;
			long long start = 0;
			while (-1 != (start = Text.find(Separator, start))) {
				if (((unsigned int)start) >= last)
					clb(Text.substr(last, start - last));
				start += ssz;
				last = start;
			}
			if (last <= Text.size())
				clb(Text.substr(last, Text.size() - last));
		}

		inline void OnSplit(const std::wstring& Text, const std::wstring& Separator, std::function<void(const std::wstring&)> clb) {
			size_t ssz = Separator.size(), last = 0;
			long long start = 0;
			while (-1 != (start = Text.find(Separator, start))) {
				if (((unsigned int)start) >= last)
					clb(Text.substr(last, start - last));
				start += ssz;
				last = start;
			}
			if (last <= Text.size())
				clb(Text.substr(last, Text.size() - last));
		}

		std::vector<std::string> Split(const std::string& Text, const std::string& Separator);
		std::vector<std::wstring> Split(const std::wstring& Text, const std::wstring& Separator);
		std::vector<std::string> Split_Trimed(const std::string& Text, const std::string& Separator);
		std::vector<std::wstring> Split_Trimed(const std::wstring& Text, const std::wstring& Separator);
		std::string Join(const std::vector<std::string>& List, const std::string& separator);
		std::wstring Join(const std::vector<std::wstring>& List, const std::wstring& separator);
		std::vector<std::string> Split_No_Empties(const std::string& Text, const std::string& Separator);
		std::vector<std::wstring> Split_No_Empties(const std::wstring& Text, const std::wstring& Separator);
		std::vector<std::string> Split_No_Empties_Trimed(const std::string& Text, const std::string& Separator);
		std::vector<std::wstring> Split_No_Empties_Trimed(const std::wstring& Text, const std::wstring& Separator);
		std::string Trim(const std::string& s);
		std::wstring Trim(const std::wstring& s);

		std::string Replace(const std::string& Text, const std::regex& pattern, std::function<const std::string(const std::smatch&)> newWordFnc);
		std::string Replace(const std::string& Text, const std::string& pattern, std::function<const std::string(const std::smatch&)> newWordFnc);

		std::wstring Replace(const std::wstring& Text, const std::wregex& pattern, std::function<const std::wstring(const std::wsmatch&)> newWordFnc);
		std::wstring Replace(const std::wstring& Text, const std::wstring& pattern, std::function<const std::wstring(const std::wsmatch&)> newWordFnc);


		std::string Replace(const std::string& Text, const std::string& pattern, const std::string& newWord);
		std::wstring Replace(const std::wstring& Text, const std::wstring& pattern, const std::wstring& newWord);

		std::string ReplaceOne(const std::string& Text, const std::string& pattern, const std::string& newWord);
		std::wstring ReplaceOne(const std::wstring& Text, const std::wstring& pattern, const std::wstring& newWord);


		std::string ToLower(const std::string& txt);
		std::wstring ToLower(const std::wstring& txt);

		std::pair<std::string, std::string> Split2(const std::string& Text, const std::string& Separator);
		std::pair<std::wstring, std::wstring> Split2(const std::wstring& Text, const std::wstring& Separator);
		std::pair<std::string, std::string> Split2_Trimed(const std::string& Text, const std::string& Separator);
		std::pair<std::wstring, std::wstring> Split2_Trimed(const std::wstring& Text, const std::wstring& Separator);


		//template<typename T, typename T2>
		//inline std::wstring Join(const std::vector<T> & List, const std::wstring & separator, T2 fnc) {
		//	std::wstringstream ret;
		//	ret << fnc(List[0]);
		//	for (auto i = 1; i < List.size(); i++)
		//		ret << separator << fnc(List[i]);
		//	return ret.str();
		//}

		template<typename T, typename T2>
		inline std::wstring Join(const T& List, const std::wstring& separator, T2 fnc) {
			if (!List.size()) return L"";
			std::wstringstream ret;

			auto iter = List.begin();

			ret << fnc(*iter);
			iter++;
			for (; iter != List.end(); iter++)
				ret << separator << fnc(*iter);
			return ret.str();
		}

		template<typename T, typename T2>
		inline std::string Join(const std::vector<T>& List, const std::string& separator, T2 fnc) {
			std::stringstream ret;
			ret << fnc(List[0]);
			for (auto i = 1; i < List.size(); i++)
				ret << separator << fnc(List[i]);
			return ret.str();
		}
	}
	namespace StringView {
		inline void OnSplit(const std::string_view& Text, const std::string_view& Separator, std::function<void(const std::string_view&)> clb) {
			size_t ssz = Separator.size(), last = 0;
			long long start = 0;
			while (-1 != (start = Text.find(Separator, start))) {
				if (((unsigned int)start) >= last)
					clb(Text.substr(last, start - last));
				start += ssz;
				last = start;
			}
			if (last <= Text.size())
				clb(Text.substr(last, Text.size() - last));
		}

		inline void OnSplit(const std::wstring_view& Text, const std::wstring_view& Separator, std::function<void(const std::wstring_view&)> clb) {
			size_t ssz = Separator.size(), last = 0;
			long long start = 0;
			while (-1 != (start = Text.find(Separator, start))) {
				if (((unsigned int)start) >= last)
					clb(Text.substr(last, start - last));
				start += ssz;
				last = start;
			}
			if (last <= Text.size())
				clb(Text.substr(last, Text.size() - last));
		}

		inline size_t Count(const std::wstring_view& Text, const std::wstring_view& Word) {
			size_t ret = 0;
			auto wsz = Word.size();
			auto i = Text.find(Word);
			while (i!=std::wstring_view::npos) {
				ret++;
				i = Text.find(Word, i + wsz);
			}
			return ret;
		}

		inline size_t Count(const std::string_view& Text, const std::string_view& Word) {
			size_t ret = 0;
			auto wsz = Word.size();
			auto i = Text.find(Word);
			while (i!=std::string_view::npos) {
				ret++;
				i = Text.find(Word, i + wsz);
			}
			return ret;
		}

		inline std::wstring_view Trim(const std::wstring_view& Text) {
			size_t Start = 0;
			size_t End = Text.size() - 1;
			while (Text[Start] == ' ' || Text[Start] == '\t' || Text[Start] == '\n' || Text[Start] == '\r') Start++;
			while (Text[End] == ' ' || Text[End] == '\t' || Text[End] == '\n' || Text[End] == '\r') End--;
			return Text.substr(Start, End - Start + 1);
		}
		inline std::string_view Trim(const std::string_view& Text) {
			size_t Start = 0;
			size_t End = Text.size() - 1;
			while (Text[Start] == ' ' || Text[Start] == '\t' || Text[Start] == '\n' || Text[Start] == '\r') Start++;
			while (Text[End] == ' ' || Text[End] == '\t' || Text[End] == '\n' || Text[End] == '\r') End--;
			return Text.substr(Start, End - Start + 1);
		}

		inline std::vector<std::wstring_view> Split(const std::wstring_view& Text, const std::wstring_view& Separator) {
			std::vector<std::wstring_view> ret;
			ret.reserve(Count(Text, Separator) + 1);
			OnSplit(Text, Separator, [&ret](const std::wstring_view& w) { ret.push_back(w); });
			return ret;
		}

		inline std::vector<std::string_view> Split(const std::string_view& Text, const std::string_view& Separator) {
			std::vector<std::string_view> ret;
			ret.reserve(Count(Text, Separator) + 1);
			OnSplit(Text, Separator, [&ret](const std::string_view& w) { ret.push_back(w); });
			return ret;
		}

		inline std::vector<std::wstring_view> Split_Trim(const std::wstring_view& Text, const std::wstring_view& Separator) {
			std::vector<std::wstring_view> ret;
			ret.reserve(Count(Text, Separator) + 1);
			OnSplit(Text, Separator, [&ret](const std::wstring_view& w) { ret.push_back(Trim(w)); });
			return ret;
		}

		inline std::vector<std::string_view> Split_Trim(const std::string_view& Text, const std::string_view& Separator) {
			std::vector<std::string_view> ret;
			ret.reserve(Count(Text, Separator) + 1);
			OnSplit(Text, Separator, [&ret](const std::string_view& w) { ret.push_back(Trim(w)); });
			return ret;
		}

		inline std::vector<std::wstring_view> Split_Trim_NoEmpties(const std::wstring_view& Text, const std::wstring_view& Separator) {
			std::vector<std::wstring_view> ret;
			ret.reserve(Count(Text, Separator) + 1);
			OnSplit(Text, Separator, [&ret](const std::wstring_view& w) {
				if (auto r = Trim(w); r.size()) ret.push_back(r);
			});
			return ret;
		}

		inline std::vector<std::string_view> Split_Trim_NoEmpties(const std::string_view& Text, const std::string_view& Separator) {
			std::vector<std::string_view> ret;
			ret.reserve(Count(Text, Separator) + 1);
			OnSplit(Text, Separator, [&ret](const std::string_view& w) {
				if (auto r = Trim(w); r.size()) ret.push_back(r);
			});
			return ret;
		}

		template<typename T>
		struct TokenDescriptor {
			T TypeId;
			std::string_view regex;
			int GroupIndex = 0;
		};
		template<typename T>
		inline std::vector<std::pair<T, std::string>> Tokenize(const std::string_view& Text, const std::initializer_list<TokenDescriptor<T>>& Types) {
			std::vector<std::tuple<T, std::regex, int>> types;
			types.reserve(Types.size());
			for (auto& t : Types) types.push_back({ t.TypeId, std::regex(t.regex.cbegin(), t.regex.cend(), std::regex_constants::optimize | std::regex_constants::syntax_option_type::ECMAScript), t.GroupIndex });
			std::vector<std::pair<T, std::string>> ret;
			std::string_view txt = Text;
			while (txt.size()) {
				for (auto& [tp, reg, Index]: types) {
					std::match_results<std::string_view::const_iterator> m;
					if (std::regex_search(txt.cbegin(), txt.cend(), m, reg); m.size()>Index && m[0].first == txt.cbegin()) {
						ret.push_back({ tp, m[Index].str() });
						size_t sz = m.str().size();
						if (!sz)
							throw "Unrecognized Token";
						txt.remove_prefix(sz);
						break;
					}
				}
			}
			return ret;
		}
	}
}

#endif