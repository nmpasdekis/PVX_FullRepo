#include<PVX_String.h>
#include<sstream>
#include<PVX_Regex.h>
#include <PVX.inl>
#include <stack>

using namespace std;

namespace PVX{
	namespace String{
		std::string Join(const std::vector<std::string> & List, const std::string & separator) {
			std::stringstream ret;
			ret << List[0];
			for(auto i = 1; i < List.size(); i++)
				ret << separator << List[i];
			return ret.str();
		}
		vector<string> Split(const string & Text, const string & Separator) {
			vector<string> ret;
			size_t ssz = Separator.size(), last = 0;
			long long start = 0;
			while(-1 != (start = Text.find(Separator, start))) {
				if(((unsigned int)start) >= last)
					ret.push_back(Text.substr(last, start - last));
				start += ssz;
				last = start;
			}
			if(last <= Text.size())
				ret.push_back(Text.substr(last, Text.size() - last));
			return ret;
		}
		vector<string> Split_No_Empties(const string & Text, const string & Separator) {
			vector<string> ret;
			OnSplit(Text, Separator, [&ret](const std::string& w) {
				if (w.size())
					ret.push_back(w);
			});
			return ret;
		}

		vector<string> Split_No_Empties_Trimed(const string & Text, const string & Separator) {
			std::vector<std::string> ret;
			OnSplit(Text, Separator, [&ret](const std::string& w) {
				auto t = Trim(w);
				if (t.size())
					ret.push_back(t);
			});
			return ret;
		}

		vector<string> Split_Trimed(const string & Text, const string & Separator) {
			std::vector<std::string> ret;
			OnSplit(Text, Separator, [&ret](const std::string& w) {
				ret.push_back(Trim(w));
			});
			return ret;
		}

		vector<wstring> Split(const wstring & Text, const wstring & Separator) {
			vector<wstring> ret;
			size_t ssz = Separator.size(), last = 0;
			long long start = 0;
			while(-1 != (start = Text.find(Separator, start))) {
				if(((unsigned int)start) >= last)
					ret.push_back(Text.substr(last, start - last));
				start += ssz;
				last = start;
			}
			if(last <= Text.size())
				ret.push_back(Text.substr(last, Text.size() - last));
			return ret;
		}
		vector<wstring> Split_No_Empties(const wstring& Text, const wstring& Separator) {
			std::vector<std::wstring> ret;
			OnSplit(Text, Separator, [&ret](const std::wstring& w) {
				if (w.size())
					ret.push_back(w);
			});
			return ret;
		}
		vector<wstring> Split_No_Empties_Trimed(const wstring & Text, const wstring & Separator) {
			std::vector<std::wstring> ret;
			OnSplit(Text, Separator, [&ret](const std::wstring& w) {
				auto t = Trim(w);
				if (t.size())
					ret.push_back(t);
			});
			return ret;
			//return PVX::Filter(PVX::Map(Split(Text, Separator), [](const std::wstring& w) { return Trim(w); }), [](const std::wstring& w) { return w.size(); });
		}
		vector<wstring> Split_Trimed(const wstring & Text, const wstring & Separator) {
			std::vector<std::wstring> ret;
			OnSplit(Text, Separator, [&ret](const std::wstring& w) {
				ret.push_back(Trim(w));
			});
			return ret;

			//return PVX::Map(Split(Text, Separator), [](const std::wstring & w) { return Trim(w); });
		}

		std::pair<std::string, std::string> Split2(const std::string& Text, const std::string& Separator) {
			if (auto index = Text.find(Separator); index!=std::string::npos) {
				return { Text.substr(0, index), Text.substr(index + Separator.size()) };
			}
			return { Text, "" };
		}
		std::pair<std::wstring, std::wstring> Split2(const std::wstring& Text, const std::wstring& Separator) {
			if (auto index = Text.find(Separator); index!=std::wstring::npos) {
				return { Text.substr(0, index), Text.substr(index + Separator.size()) };
			}
			return { Text, L"" };
		}
		std::pair<std::string, std::string> Split2_Trimed(const std::string& Text, const std::string& Separator) {
			if (auto index = Text.find(Separator); index!=std::string::npos) {
				return { Trim(Text.substr(0, index)), Trim(Text.substr(index + Separator.size())) };
			}
			return { Trim(Text), "" };
		}
		std::pair<std::wstring, std::wstring> Split2_Trimed(const std::wstring& Text, const std::wstring& Separator) {
			if (auto index = Text.find(Separator); index!=std::wstring::npos) {
				return { Trim(Text.substr(0, index)), Trim(Text.substr(index + Separator.size())) };
			}
			return { Trim(Text), L"" };
		}


		std::wstring Join(const std::vector<std::wstring> & List, const std::wstring & separator) {
			std::wstringstream ret;
			if (List.size()) {
				ret << List[0];
				for (auto i = 1; i < List.size(); i++)
					ret << separator << List[i];
			}
			return ret.str();
		}

		string Trim(const string & s) {
			long long start, end;
			for(start = 0; s[start] && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r'); start++);
			for(end = s.size() - 1; end >= 0 && (s[end] == ' ' || s[end] == '\t' || s[end] == '\n' || s[end] == '\r'); end--);
			return s.substr(start, end - start + 1);
		}
		wstring Trim(const wstring & s) {
			long long start, end;
			for(start = 0; s[start] && (s[start] == L' ' || s[start] == L'\t' || s[start] == L'\n' || s[start] == L'\r'); start++);
			for(end = s.size() - 1; end >= 0 && (s[end] == L' ' || s[end] == L'\t' || s[end] == L'\n' || s[end] == L'\r'); end--);
			return s.substr(start, end - start + 1);
		}

		std::string Replace(const std::string & Text, const std::regex & pattern, std::function<const std::string(const std::smatch&)> newWordFnc) {
			std::vector<smatch> matches;
			std::string ret(Text);
			PVX::onMatch(Text, pattern, [&matches, &Text](const std::smatch & m) {
				matches.push_back(m);
			}); for (long long i = matches.size() - 1; i >= 0; i--) {
				ret.replace(matches[i][0].first._Ptr - Text.c_str(), matches[i][0].second._Ptr - matches[i][0].first._Ptr, newWordFnc(matches[i]).c_str());
			}
			return ret;
		}
		std::string Replace(const std::string & Text, const std::string & pattern, std::function<const std::string(const std::smatch&)> newWordFnc) {
			return Replace(Text, std::regex(pattern, std::regex_constants::optimize), newWordFnc);
		}

		std::wstring Replace(const std::wstring& Text, const std::wregex& pattern, std::function<const std::wstring(const std::wsmatch&)> newWordFnc) {
			std::vector<wsmatch> matches;
			std::wstring ret(Text);
			PVX::onMatch(Text, pattern, [&matches, &Text](const std::wsmatch& m) {
				matches.push_back(m);
			}); for (long long i = matches.size() - 1; i >= 0; i--) {
				ret.replace(matches[i][0].first._Ptr - Text.c_str(), matches[i][0].second._Ptr - matches[i][0].first._Ptr, newWordFnc(matches[i]).c_str());
			}
			return ret;
		}
		std::wstring Replace(const std::wstring& Text, const std::wstring& pattern, std::function<const std::wstring(const std::wsmatch&)> newWordFnc) {
			return Replace(Text, std::wregex(pattern, std::regex_constants::optimize), newWordFnc);
		}

		std::string Replace(const std::string& Text, const std::string& pattern, const std::string& newWord) {
			std::string ret(Text);
			auto osz = pattern.size();
			auto nsz = newWord.size();
			auto index = ret.find(pattern, 0);
			while (index != std::string::npos) {
				ret.replace(index, osz, newWord);
				index = ret.find(pattern, index + nsz);
			}
			return ret;
		}

		std::wstring Replace(const std::wstring& Text, const std::wstring& pattern, const std::wstring& newWord) {
			std::wstring ret(Text);
			auto osz = pattern.size();
			auto nsz = newWord.size();
			auto index = ret.find(pattern, 0);
			while (index != std::string::npos) {
				ret.replace(index, osz, newWord);
				index = ret.find(pattern, index + nsz);
			}
			return ret;
		}

		std::string ReplaceOne(const std::string& Text, const std::string& pattern, const std::string& newWord) {
			std::string ret(Text);
			auto osz = pattern.size();
			auto nsz = newWord.size();
			auto index = ret.find(pattern, 0);
			while (index != std::string::npos) {
				ret.replace(index, osz, newWord);
				index = ret.find(pattern, index + nsz);
			}
			return ret;
		}

		std::wstring ReplaceOne(const std::wstring& Text, const std::wstring& pattern, const std::wstring& newWord) {
			std::wstring ret(Text);
			auto osz = pattern.size();
			auto index = ret.find(pattern, 0);
			if (index != std::string::npos) {
				ret.replace(index, osz, newWord);
			}
			return ret;
		}


		std::string ToLower(const std::string & txt) {
			std::string ret;
			for (auto & c : txt) ret.push_back(tolower(c));
			return ret;
		}

		std::wstring ToLower(const std::wstring & txt) {
			std::wstring ret;
			for (auto & c : txt) ret.push_back(tolower(c));
			return ret;
		}
	}
}