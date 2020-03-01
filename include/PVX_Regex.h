#ifndef __PVX_REGEX_H__
#define __PVX_REGEX_H__

#include <vector>
#include <regex>
#include <string>
#include <string_view>
#include <functional>

namespace PVX {
	inline std::smatch regex_match(const std::string &Text, const std::regex & RegExp) {
		auto ret = std::sregex_iterator(Text.cbegin(), Text.cend(), RegExp);
		if(ret != std::sregex_iterator())
			return *ret;
		return std::smatch();
	}
	inline std::wsmatch regex_match(const std::wstring &Text, const std::wregex & RegExp) {
		auto ret = std::wsregex_iterator(Text.cbegin(), Text.cend(), RegExp);
		if (ret != std::wsregex_iterator()) 
			return *ret;
		return std::wsmatch();
	}
	inline std::smatch regex_match(const std::string &Text, const std::string & RegExp) {
		return regex_match(Text, std::regex(RegExp));
	}
	inline std::wsmatch regex_match(const std::wstring &Text, const std::wstring & RegExp) {
		return regex_match(Text, std::wregex(RegExp));
	}

	inline decltype(*std::regex_iterator<std::string_view::const_iterator>()) regex_match(const std::string_view& Text, const std::regex& RegExp) {
		using iter = std::regex_iterator<std::string_view::const_iterator>;
		auto ret = iter(Text.cbegin(), Text.cend(), RegExp);
		if (ret != iter())
			return *ret;
		return {};
	}

	inline std::vector<std::smatch> regex_matches(const std::string &Text, const std::regex & RegExp) {
		std::vector<std::smatch> ret;
		for(auto it = std::sregex_iterator(Text.cbegin(), Text.cend(), RegExp); it != std::sregex_iterator(); it++)
			ret.push_back(*it);
		return ret;
	}

	inline std::vector<std::wsmatch> regex_matches(const std::wstring &Text, const std::wregex & RegExp) {
		std::vector<std::wsmatch> ret;
		for(auto it = std::wsregex_iterator(Text.cbegin(), Text.cend(), RegExp); it != std::wsregex_iterator(); it++)
			ret.push_back(*it);
		return ret;
	}

	inline std::vector<std::smatch> regex_matches(const std::string &Text, const std::string & RegExp) {
		return regex_matches(Text, std::regex(RegExp, std::regex_constants::optimize));
	}

	inline std::vector<std::wsmatch> regex_matches(const std::wstring &Text, const std::wstring & RegExp) {
		return regex_matches(Text, std::wregex(RegExp, std::regex_constants::optimize));
	}

	inline void onMatch(const std::string & Text, const std::regex & pattern, std::function<void(const std::smatch &m)> fnc) {
		for (auto it = std::sregex_iterator(Text.cbegin(), Text.cend(), pattern); it != std::sregex_iterator(); it++) fnc(*it);
	}

	inline void onMatch(const std::wstring & Text, const std::wregex & pattern, std::function<void(const std::wsmatch &m)> fnc) {
		for (auto it = std::wsregex_iterator(Text.cbegin(), Text.cend(), pattern); it != std::wsregex_iterator(); it++) fnc(*it);
	}

	inline void onMatch(const std::string & Text, const std::string & pattern, std::function<void(const std::smatch &m)> fnc) {
		std::regex reg(pattern, std::regex_constants::optimize);
		for (auto it = std::sregex_iterator(Text.cbegin(), Text.cend(), reg); it != std::sregex_iterator(); it++) fnc(*it);
	}

	inline void onMatch(const std::wstring & Text, const std::wstring & pattern, std::function<void(const std::wsmatch &m)> fnc) {
		std::wregex reg(pattern, std::regex_constants::optimize);
		for (auto it = std::wsregex_iterator(Text.cbegin(), Text.cend(), reg); it != std::wsregex_iterator(); it++) fnc(*it);
	}


	template<typename T= std::smatch>
	inline std::vector<T> regex_matches(const std::string &Text, const std::regex & RegExp, std::function<T(const std::smatch&)> f) {
		std::vector<T> ret;
		for(auto it = std::sregex_iterator(Text.cbegin(), Text.cend(), RegExp); it != std::sregex_iterator(); it++)
			ret.push_back(f(*it));
		return ret;
	}

	template<typename T = std::wsmatch>
	inline std::vector<T> regex_matches(const std::wstring &Text, const std::wregex & RegExp, std::function<T(const std::wsmatch&)> f) {
		std::vector<T> ret;
		for(auto it = std::wsregex_iterator(Text.cbegin(), Text.cend(), RegExp); it != std::wsregex_iterator(); it++)
			ret.push_back(f(*it));
		return ret;
	}

	template<typename T = std::smatch>
	inline std::vector<T> regex_matches(const std::string &Text, const std::string & regExp, std::function<T(const std::smatch&)> f) {
		std::vector<T> ret;
		std::regex reg(regExp, std::regex_constants::optimize);
		for(auto it = std::sregex_iterator(Text.cbegin(), Text.cend(), reg); it != std::sregex_iterator(); it++)
			ret.push_back(f(*it));
		return ret;
	}

	template<typename T = std::wsmatch>
	inline std::vector<T> regex_matches(const std::wstring &Text, const std::wstring & regExp, std::function<T(const std::wsmatch&)> f) {
		std::vector<T> ret;
		std::wregex reg(regExp, std::regex_constants::optimize);
		for(auto it = std::wsregex_iterator(Text.cbegin(), Text.cend(), reg); it != std::wsregex_iterator(); it++)
			ret.push_back(f(*it));
		return ret;
	}

	template<typename T_wsmatch>
	auto regex_matches(const std::wstring& Text, const std::wstring& regExp, T_wsmatch f) {
		std::vector<decltype(f(std::wsmatch()))> ret;
		std::wregex reg(regExp, std::regex_constants::optimize);
		for (auto it = std::wsregex_iterator(Text.cbegin(), Text.cend(), reg); it != std::wsregex_iterator(); it++)
			ret.push_back(f(*it));
		return ret;
	}

	inline std::string Replace(const std::string & Input, const std::string & regExp, const std::string && replaceWith) {
		return std::regex_replace(Input, std::regex(regExp, std::regex_constants::optimize), replaceWith);
	}

	inline std::wstring Replace(const std::wstring & Input, const std::wstring & regExp, const std::wstring && replaceWith) {
		return std::regex_replace(Input, std::wregex(regExp, std::regex_constants::optimize), replaceWith);
	}



	inline std::string Replace(const std::string& Input, const std::string& regExp, std::function<std::string(const std::smatch&)> replaceWith) {
		auto matches = regex_matches<std::pair<std::smatch, std::string>>(Input, std::regex(regExp, std::regex_constants::optimize), [&replaceWith, &Input](const std::smatch& m) {
			return std::make_pair(m, replaceWith(m));
		});

		std::string ret = Input;
		for (long long i = matches.size()-1; i>=0; i--) {
			auto& m = matches[i];
			ret = ret.replace(m.first.position(), m.first.size(), m.second);
		}
		return ret;
	}
	inline std::wstring Replace(const std::wstring& Input, const std::wstring& regExp, std::function<std::wstring(const std::wsmatch&)> replaceWith) {
		auto matches = regex_matches<std::pair<std::wsmatch, std::wstring>>(Input, std::wregex(regExp, std::regex_constants::optimize), [&replaceWith](const std::wsmatch& m) {
			return std::make_pair(m, replaceWith(m));
		});

		std::wstring ret = Input;
		for (long long i = matches.size()-1; i>=0; i--) {
			auto& m = matches[i];
			ret = ret.replace(m.first.position(), m.first.str().size(), m.second);
		}
		return ret;
	}
}

#endif