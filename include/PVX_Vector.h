#pragma once
#include <vector>

namespace PVX {
	template<typename T>
	class vector {
		std::vector<T> Data;
	public:
		template<typename arg> vector(arg a) :Data(a) {}
		vector() :Data{} {}
		vector(const std::initializer_list<T>& args) : Data(args) {};
		T& operator[](size_t i) { return Data[i]; }
		const T& operator[](size_t i) const { return Data[i]; }

		size_t size() const { return Data.size(); }
		void reserve(size_t sz) { Data.reserve(sz); }
		void resize(size_t sz) { Data.resize(sz); }

		vector<T>& push_back(const T& it) {
			Data.push_back(it);
			return *this;
		}
		T& emplace_back(const T& it) {
			return Data.emplace_back(it);
		}
		T& back() { return Data.back(); }
		const T& back() const { return Data.back(); }
		T& front() { return Data.front(); }
		const T& front() const { return Data.front(); }
		template<typename fnc_t>
		auto map(fnc_t clb) const {
			vector<decltype(clb(Data[0]))> ret;
			ret.reserve(Data.size());
			for (const T& it: Data) ret.push_back(clb(it));
			return ret;
		}
		template<typename fnc_t>
		auto filter(fnc_t clb) const {
			vector<T> ret;
			ret.reserve(Data.size());
			for (const T& it: Data) if(clb(it)) ret.push_back(it);
			return ret;
		}
	};
}