#include<PVX_String.h>
#include<sstream>
#include<PVX_Regex.h>
#include <PVX_Encode.h>
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
			int64_t start = 0;
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
			int64_t start = 0;
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
			int64_t start, end;
			for(start = 0; s[start] && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r'); start++);
			for(end = s.size() - 1; end >= 0 && (s[end] == ' ' || s[end] == '\t' || s[end] == '\n' || s[end] == '\r'); end--);
			return s.substr(start, end - start + 1);
		}
		wstring Trim(const wstring & s) {
			int64_t start, end;
			for(start = 0; s[start] && (s[start] == L' ' || s[start] == L'\t' || s[start] == L'\n' || s[start] == L'\r'); start++);
			for(end = s.size() - 1; end >= 0 && (s[end] == L' ' || s[end] == L'\t' || s[end] == L'\n' || s[end] == L'\r'); end--);
			return s.substr(start, end - start + 1);
		}

		std::string Replace(const std::string & Text, const std::regex & pattern, std::function<const std::string(const std::smatch&)> newWordFnc) {
			std::vector<smatch> matches;
			std::string ret(Text);
			PVX::onMatch(Text, pattern, [&matches](const std::smatch & m) {
				matches.push_back(m);
			}); 
			for (int64_t i = matches.size() - 1; i >= 0; i--) {
				//ret.replace(matches[i][0].first._Ptr - Text.c_str(), matches[i][0].second._Ptr - matches[i][0].first._Ptr, newWordFnc(matches[i]).c_str());
				ret.replace(matches[i][0].first.operator->() - Text.c_str(), matches[i][0].second.operator->() - matches[i][0].first.operator->(), newWordFnc(matches[i]).c_str());
			}
			return ret;
		}
		std::string Replace(const std::string & Text, const std::string & pattern, std::function<const std::string(const std::smatch&)> newWordFnc) {
			return Replace(Text, std::regex(pattern, std::regex_constants::optimize), newWordFnc);
		}

		std::wstring Replace(const std::wstring& Text, const std::wregex& pattern, std::function<const std::wstring(const std::wsmatch&)> newWordFnc) {
			std::vector<wsmatch> matches;
			std::wstring ret(Text);
			PVX::onMatch(Text, pattern, [&matches](const std::wsmatch& m) {
				matches.push_back(m);
			}); for (int64_t i = matches.size() - 1; i >= 0; i--) {
				//ret.replace(matches[i][0].first._Ptr - Text.c_str(), matches[i][0].second._Ptr - matches[i][0].first._Ptr, newWordFnc(matches[i]).c_str());
				ret.replace(matches[i][0].first.operator->() - Text.c_str(), matches[i][0].second.operator->() - matches[i][0].first.operator->(), newWordFnc(matches[i]).c_str());
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
			ret.reserve(txt.size());
			for (auto & c : txt) ret.push_back(std::tolower(c));
			return ret;
		}

		std::wstring ToLower(const std::wstring & txt) {
			std::wstring ret;
			ret.reserve(txt.size());
			for (auto & c : txt) ret.push_back(std::tolower(c));
			return ret;
		}

		std::string ToUpper(const std::string& txt) {
			std::string ret;
			ret.reserve(txt.size());
			for (auto& c : txt) ret.push_back(std::toupper(c));
			return ret;
		}

		std::wstring ToUpper(const std::wstring& txt) {
			std::wstring ret;
			ret.reserve(txt.size());
			for (auto& c : txt) ret.push_back(std::toupper(c));
			return ret;
		}

		std::wstring removeAccent(const std::wstring& accentedStr) {
			const std::map<wchar_t, wchar_t> accent{ { L'ά', L'α' }, { L'έ', L'ε' }, { L'ί', L'ι' }, { L'ϊ', L'ι' }, { L'ΐ', L'ι' }, { L'ή', L'η' }, { L'ύ', L'υ' }, { L'ϋ', L'υ' }, { L'ΰ', L'υ' }, { L'ό', L'ο' }, { L'ώ', L'ω' }, { L'Ά', L'α' }, { L'Έ', L'ε' }, { L'Ί', L'ι' }, { L'Ϊ', L'ι' }, { L'ΐ', L'ι' }, { L'Ή', L'η' }, { L'Ύ', L'υ' }, { L'Ϋ', L'υ' }, { L'ΰ', L'υ' }, { L'Ό', L'ο' }, { L'Ώ', L'ω' }, { L'ς', L'σ' }, { L'σ', L'σ' } };
			std::wstring ret; ret.reserve(accentedStr.size());
			for (auto c : accentedStr) ret.push_back(GetOrDefault(accent, c, wchar_t(c | (L'a'^L'A'))));
			return ret;
		}

		static std::vector<int> _cmpStrElem;
		static std::vector<int*> _cmpStrRows;
		static int** MakeArray2D(int r, int c) {
			if (_cmpStrElem.size() < (r * c))_cmpStrElem.resize(r * c);
			if (_cmpStrRows.size() < r)_cmpStrRows.resize(r);
			_cmpStrRows[0] = _cmpStrElem.data();
			for (int i = 1; i < r; i++) _cmpStrRows[i] = _cmpStrRows[i - 1] + c;
			return _cmpStrRows.data();
		}


		//int EditDistance_AccentSensitive(const std::wstring& s, const std::wstring& t) {
		//	auto n = s.size();
		//	auto m = t.size();

		//	if (n == 0) return m;
		//	if (m == 0) return n;

		//	auto d = MakeArray2D(n + 1, m + 1);

		//	for (int i = 0; i <= n; i++) d[i][0] = i;
		//	for (int j = 0; j <= m; j++) d[0][j] = j;

		//	for (int i = 1; i <= n; i++) for (int j = 1; j <= m; j++) {
		//		d[i][j] = std::min(std::min(d[i - 1][j] + 1, d[i][j - 1] + 1), d[i - 1][j - 1] + ((t[j - 1] == s[i - 1]) ? 0 : 1));
		//	}
		//	return d[n][m];
		//}

		int EditDistance_AccentSensitive(const std::wstring& S, const std::wstring& T) {
			auto n = S.size();
			auto m = T.size();

			if (n == 0) return m;
			if (m == 0) return n;

			const wchar_t* s = S.data();
			const wchar_t* t = T.data();
			if (m > n) {
				t = S.data();
				m = S.size();
				s = T.data();
				n = T.size();
			}

			auto d = MakeArray2D(2, m + 1);

			for (int j = 0; j <= m; j++) d[0][j] = j;
			int cur = 0;

			for (int i = 1; i <= n; i++) {
				cur ^= 1;
				int* Prev = d[cur^1];
				int* This = d[cur];
				This[0] = i;
				for (int j = 1; j <= m; j++) {
					This[j] = std::min(std::min(Prev[j] + 1, This[j - 1] + 1), Prev[j - 1] + ((t[j - 1] == s[i - 1]) ? 0 : 1));
				}
			}
			return d[cur][m];
		}

		int EditDistance(const std::wstring& S, const std::wstring& T) {
			auto n = S.size();
			auto m = T.size();
			if (!n) return m;
			if (!m) return n;
			return EditDistance_AccentSensitive(removeAccent(S), removeAccent(T));
		}



		int LongestCommonSubstring_AccentSensitive(const std::wstring& A, const std::wstring& B) {
			int n = A.size(), m = B.size();
			if (m==0||n==0) return 0;
			const wchar_t *a = A.c_str(), *b = B.c_str();
			if (m > n) {
				b = A.c_str();
				a = B.c_str();
				m = n;
				n = B.size();
			}

			auto d = MakeArray2D(2, m + 1);
			memset(d[0], 0, (m+1) * sizeof(int));
			int cur = 0;

			int mx = 0;
			for (int i = 0; i < n; i++){
				int* upLine = d[cur];
				int* Line = d[cur^1];
				Line[0] = 0;
				for (int j = 0; j < m; j++) {
					if (a[i] == b[j]) {
						mx = std::max(mx, Line[j + 1] = upLine[j] + 1);
					} else {
						Line[j + 1] = 0;
					}
				}
				cur ^= 1;
			}
			return mx;
		}

		int LongestCommonSubstring(const std::wstring& A, const std::wstring& B) {
			if (A.size()&&B.size())
				return LongestCommonSubstring_AccentSensitive(removeAccent(A), removeAccent(B));
			return 0;
		}

		std::tuple<int, int> FindLongestCommonSubstring_AccentSensitive(const std::wstring_view& a, const std::wstring_view& b) {
			int n = a.size(), m = b.size();
			if (m==0||n==0) return { 0, 0 };
			int lastIndexA = 0, lastIndexB = 0, mx = 0;
			int cur = 0;
			auto d = MakeArray2D(2, n + 1);
			memset(d[0], 0, (n + 1) * sizeof(int));

			for (int i = 0; i < m; i++) {
				int* upLine = d[cur];
				int* Line = d[cur^1];
				Line[0] = 0;
				for (int j = 0; j < n; j++) {
					if (a[j] == b[i]) {
						if(int& val = Line[j + 1] = upLine[j] + 1; mx < val) { mx = val; lastIndexA = j; lastIndexB = i; }
					} else {
						Line[j + 1] = 0;
					}
				}
				cur ^= 1;
			}
			return { lastIndexA - mx + 1, lastIndexB - mx + 1 };
		}
	}
}