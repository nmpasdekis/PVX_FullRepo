#pragma once
#include <vector>
#include <map>
#include <functional>
#include <random>
#include <initializer_list>
#include <set>
#include <algorithm>
#include <future>
#include <type_traits>
#include <numeric>
#include <memory>
#include <chrono>

#ifdef _USE_PARALLEL_
#include <execution>
#endif

#ifdef __PVX_InitialValue__
#define __InitialValue1__(x) x
#define __InitialValue2__(x) __InitialValue1__(x), __InitialValue1__(x)
#define __InitialValue4__(x) __InitialValue2__(x), __InitialValue2__(x)
#define __InitialValue8__(x) __InitialValue4__(x), __InitialValue4__(x)
#define __InitialValue16__(x) __InitialValue8__(x), __InitialValue8__(x)
#define __InitialValue32__(x) __InitialValue16__(x), __InitialValue16__(x)
#define __InitialValue64__(x) __InitialValue32__(x), __InitialValue32__(x)
#define __InitialValue128__(x) __InitialValue64__(x), __InitialValue64__(x)
#define __InitialValue256__(x) __InitialValue128__(x), __InitialValue128__(x)
#define __InitialValue512__(x) __InitialValue256__(x), __InitialValue256__(x)
#define __InitialValue1024__(x) __InitialValue512__(x), __InitialValue512__(x)
#endif

//#ifdef __linux
//#include <PVX_Encode.h>
//#define fread_s(data, buf, size, count, file) fread(data, size, count, file)
//#define memcpy_s(dest, dstSize, src, byteCount) memcpy(dest, src, byteCount)
//
//inline bool _wfopen_s(FILE** fl, const wchar_t* filename, const wchar_t* mode) {
//	char md[5];
//	int i;
//	for (i = 0; mode[i]; i++) md[i] = char(mode[i]); md[i] = 0;
//	auto fn = PVX::Encode::UtfString(filename);
//	*fl = fopen(fn.c_str(), md);
//	return *fl == nullptr;
//}
//inline bool fopen_s(FILE** fl, const char* filename, const char* mode) {
//	*fl = fopen(filename, mode);
//	return *fl == nullptr;
//}
//#endif


namespace PVX {
	class Timer {
		decltype(std::chrono::high_resolution_clock::now()) lastTime = std::chrono::high_resolution_clock::now();
	public:
		inline float get() {
			auto now = std::chrono::high_resolution_clock::now();
			float ret = float((now - lastTime).count() / 1000000000.0);
			lastTime = now;
			return ret;
		}
	};

	class RandomFloat {
		std::mt19937 Engine{ [] {
			std::mt19937 ret;
			ret.seed(std::random_device()());
			return ret;
		}() };
		std::uniform_real_distribution<float> Distribution;
	public:
		inline float NextPositive() { return Distribution(Engine); }
		inline float Next() { return Distribution(Engine) * 2.0f - 1.0f; }
	};

	class RandomInt {
		std::mt19937 Engine{ [] {
			std::mt19937 ret;
			ret.seed(std::random_device()());
			return ret;
		}() };
		std::uniform_int_distribution<int> Distribution;
	public:
		inline int Next() { return Distribution(Engine); }
		inline int Next(int min, int max) { return (Distribution(Engine)%(max-min+1)) + min; }
	};

	template<typename clbType, typename RetType, typename defClb>
	RetType find(const std::vector<RetType>& list, clbType clb, defClb def) {
		for (const auto& it : list) {
			if (clb(it)) return it;
		}
		return def();
	}

	template<typename clbType, typename RetType>
	RetType find(const std::vector<RetType>& list, clbType clb, RetType def) {
		for (const auto& it : list) {
			if (clb(it)) return it;
		}
		return def;
	}

	inline void ForEach(size_t count, std::function<void(size_t)> clb) {
		for (auto i = 0; i<count; i++)clb(i);
	}

	template<typename T>
	inline std::vector<T> ToVector(const T* Array, size_t Count) {
		std::vector<T> ret;
		ret.reserve(Count);
		for (auto i = 0; i<Count; i++)
			ret.push_back(Array[i]);
		//memcpy(&ret[0], Array, sizeof(T) * Count);
		return std::move(ret);
	}

	template<typename T, typename T2>
	inline std::vector<T> Filter(const std::initializer_list<T>& Array, T2 condition) {
		std::vector<T> ret;
		ret.reserve(Array.size());
		for (auto& a : Array)
			if (condition(a))
				ret.push_back(a);
		ret.shrink_to_fit();
		return std::move(ret);
	}
	template<typename T, typename T2>
	inline std::vector<T> Filter(const std::vector<T>& Array, T2 condition) {
		std::vector<T> ret;
		ret.reserve(Array.size());
		for (auto& a : Array)
			if (condition(a))
				ret.push_back(a);
		ret.shrink_to_fit();
		return std::move(ret);
	}
	template<typename T, typename T2>
	inline std::pair<std::vector<T>, std::vector<T>> Separate(const std::vector<T>& Array, T2 condition) {
		std::vector<T> ret1;
		std::vector<T> ret2;
		ret1.reserve(Array.size());
		ret2.reserve(Array.size());
		for (auto& a : Array) {
			if (condition(a))
				ret1.push_back(a);
			else
				ret2.push_back(a);
		}
		ret1.shrink_to_fit();
		ret2.shrink_to_fit();
		return std::make_pair(ret1, ret2);
	}
	template<typename T>
	inline int64_t IndexOf(const std::vector<T>& Array, std::function<bool(decltype(Array[0])& a)> fnc) {
		for (auto i = 1; i < Array.size(); i++) {
			if (fnc(Array[i]))
				return (int64_t)i;
		}
		return -1;
	}
	std::vector<int> IndexArray(int From, int To, int Step = 1);

	inline std::vector<int> IndexArray(int From, int To, int Step) {
		std::vector<int> ret;
		ret.reserve(size_t(To) - From);
		for (auto i = From; i < To; i += Step) {
			ret.push_back(i);
		}
		return std::move(ret);
	}
	inline std::vector<int> IndexArray(int Count) {
		std::vector<int> ret(Count);
		for (auto i = 0; i < Count; i++) {
			ret[i] = i;
		}
		return std::move(ret);
	}
	inline std::vector<int> IndexArrayRef(int Count, int& Base) {
		std::vector<int> ret(Count);
		for (auto i = 0; i < Count; i++)
			ret[i] = Base++;
		return std::move(ret);
	}

	template<typename T>
	inline void Randomize(std::vector<T>& v) {
		std::default_random_engine gen;
		std::normal_distribution dist(0, v.size()-1);
		for (int i = 0; i < v.size(); i++) {
			int r = dist(gen);
			if (i != r) std::swap(v[i], v[r]);
		}
	}

	template<typename T1, typename T2>
	inline auto Map(const T1& Array, size_t Count, T2 cvt, size_t Start = 0, size_t Size = 0);
	template<typename T1, typename T2>
	inline auto Map(const T1& Array, size_t Count, T2 cvt, size_t Start, size_t Size) {
		if (!Size)Size = Count;
		std::vector<decltype(cvt(Array[0]))> ret(Count);
		for (auto i = Start; i < Size; i++)
			ret[i] = cvt(Array[i]);
		return std::move(ret);
	}

	//template<typename T1, typename T2>
	//inline auto Map(const T1 * Array, size_t Count, T2 cvt, size_t Start = 0, size_t Size = 0);
	//template<typename T1, typename T2>
	//inline auto Map(const T1 * Array, size_t Count, T2 cvt, size_t Start, size_t Size) {
	//	if(!Size)Size = Count;
	//	std::vector<decltype(cvt(Array[0]))> ret(Count);
	//	for(auto i = Start; i < Size; i++)
	//		ret[i] = cvt(Array[i]);
	//	return ret;
	//}

	template<typename T>
	inline auto nMap(size_t Count, T fnc, const size_t Base = 0);
	template<typename T>
	inline auto nMap(size_t Count, T fnc, const size_t Base) {
		std::vector<decltype(fnc(0))> ret(Count - Base);
		for (auto i = Base; i < Count; i++) {
			ret[i - Base] = fnc(i);
		}
		return std::move(ret);
	}

	template<typename T1, typename T2>
	inline auto Map(const T1& Array, T2 cvt, size_t Start, size_t Size = 0);

	template<typename T1, typename T2>
	inline auto Map(const T1& Array, T2 cvt, size_t Start, size_t Size) {
		if (!Size)Size = Array.size();
		std::vector<decltype(cvt(Array[0]))> ret;
		ret.reserve(Array.size());
		for (auto i = Start; i < Size; i++)
			ret.push_back(cvt(Array[i]));
		return std::move(ret);
	}

	template<typename T, typename T2>
	void forEach(const std::vector<T>& Array, T2 fnc) {
		std::for_each(Array.begin(), Array.end(), fnc);
	}
#ifdef _USE_PARALLEL_
	template<typename T>
	void forEach_Parallel(const std::vector<T>& Array, std::function<void(T&)> fnc) {
		std::for_each(std::execution::par, Array.begin(), Array.end(), fnc);
	}
#endif
	template<typename KeyType, typename ValueType>
	void forEach(const std::map<KeyType, ValueType>& Map, std::function<void(const KeyType&, const ValueType&)> fnc) {
		std::for_each(Map.begin(), Map.end(), [fnc](const std::pair<KeyType, ValueType>& p) {
			fnc(p.first, p.second);
		});
	}

	template<typename T1, typename T2>
	inline auto Map(const std::vector<T1>& Array, T2 fnc) {
		std::vector<decltype(fnc(Array[0]))> ret;
		ret.reserve(Array.size());
		//std::transform(Array.begin(), Array.end(), std::back_insert_iterator(ret), fnc);
		for (auto& a : Array) ret.push_back(fnc(a));
		return std::move(ret);
	}
	//template<typename T1, typename T2, typename = std::enable_if<std::is_integral<T1>::value>>
	//auto Map(const T1& Array, T2 clb) {
	//	std::vector<decltype(clb(Array.begin()))> ret;
	//	ret.reserve(Array.size());
	//	for (auto& x: Array) ret.emplace_back(clb(x));
	//	return std::move(ret);
	//}
#ifdef _USE_PARALLEL_
	template<typename T1, typename T2>
	inline auto Map_Parallel2(const std::vector<T1>& Array, T2 fnc) {
		using returnType = decltype(fnc(Array[0]));

		std::vector<returnType> ret(Array.size());
		std::for_each(std::execution::par, Array.begin(), Array.end(), [&ret, &Array, fnc](const T1& it) {
			auto Index = &it - Array.data();
			ret[Index] = fnc(it);
		});
		return ret;
	}
#endif

	template<typename T>
	bool All(const T& Container, std::function<bool(const decltype(Container[0])&)> pred) {
		return std::all_of(Container.begin(), Container.end(), pred);
	}

	inline bool All(size_t Size, std::function<bool(size_t)> pred) {
		for (size_t i = 0; i<Size; i++) if (!pred(i))return false;
		return true;
	}

	template<typename T>
	bool Any(const T& Container, std::function<bool(const decltype(Container[0])&)> pred) {
		return std::any_of(Container.begin(), Container.end(), pred);
	}
	inline bool Any(size_t Size, std::function<bool(size_t)> pred) {
		for (size_t i = 0; i<Size; i++) if (pred(i))return true;
		return false;
	}

	template<typename T>
	void SortInplace(T& container, std::function<bool(const decltype(container[0])&, const decltype(container[0])&)> pred) {
		std::sort(container.begin(), container.end(), pred);
	}
	template<typename T>
	T&& Sort(T&& container, std::function<bool(const decltype(container[0])&, const decltype(container[0])&)> pred) {
		std::sort(container.begin(), container.end(), pred);
		return std::move(container);
	}

	template<typename T1, typename T2>
	inline auto Map_Parallel(const std::vector<T1>& Array, T2 fnc) {
		using returnType = decltype(fnc(Array[0]));

		std::vector<std::future<returnType>> promises;
		promises.reserve(Array.size());
		for (auto& it : Array) {
			promises.push_back(std::async(std::launch::async, fnc, it));
		}
		std::vector<returnType> ret;
		ret.reserve(Array.size());
		for (auto& p : promises) {
			ret.push_back(p.get());
		}
		return ret;
	}

	template<typename T1, typename T2, typename = std::enable_if_t<std::is_integral<T1>::value>>
	inline auto Map(T1 count, T2 fnc) {
		std::vector<decltype(fnc(0))> ret;
		ret.reserve(count);
		for (auto i = 0; i<count; i++)
			ret.push_back(fnc(i));
		return ret;
	}

	template<typename T1, typename T2>
	inline std::vector<T1> Keys(const std::map<T1, T2>& dict) {
		std::vector<T1> ret;
		for (auto& kv : dict) {
			ret.push_back(kv.first);
		}
		return std::move(ret);
	}
	template<typename T>
	inline std::set<T> ToSet(const std::vector<T>& arr) {
		std::set<T> ret;
		for (auto& i : arr)ret.insert(i);
		return std::move(ret);
	}

	template<typename T1, typename T2>
	inline auto ToSet(const T1& Array, T2 fnc) {
		std::set<decltype(fnc(Array[0]))> ret;
		for (auto& i : Array)ret.insert(fnc(i));
		return std::move(ret);
	}

	template<typename Container, typename result, typename Operation>
	inline result Reduce(const Container& Array, const result& Default, Operation Op) {
		return std::accumulate(Array.begin(), Array.end(), Default, Op);
	}

	class RefCounter {
		int* ref;
	public:
		inline RefCounter() : ref{ new int(1) } {}
		inline RefCounter(RefCounter&& r) : ref{ r.ref } { (*ref)++; }
		inline RefCounter(const RefCounter& r) : ref{ r.ref } { (*ref)++; }
		inline ~RefCounter() {
			if (!--(*ref)) delete ref;
		}
		inline RefCounter& operator=(const RefCounter& r) {
			if (ref && !--(*ref)) delete ref;
			ref = r.ref;
			(*ref)++;
			return *this;
		}
		inline operator int() { return *ref; }
		inline bool operator!() { return !((*ref)-1); }
		inline int operator++() { return (*ref)++; }
	};

	inline int64_t IndexOfBinary(const unsigned char* Data, int64_t DataSize, const unsigned char* Find, int64_t FindSize, int64_t Start = 0) {
		DataSize -= FindSize;
		for (int64_t i = Start; i < DataSize; i++) {
			if (!std::memcmp(Data + i, Find, FindSize)) return i;
		}
		return -1;
	}
	inline void Interleave(void* dst, size_t dstStride, const void* src, size_t srcStride, size_t Count) {
		auto min = srcStride < dstStride ? srcStride : dstStride;

		unsigned char* Dst = (unsigned char*)dst;
		const unsigned char* Src = (const unsigned char*)src;
		for (int i = 0; i < Count; i++) {
			std::memcpy(Dst, Src, min);
			Dst += dstStride;
			Src += srcStride;
		}
	}

	template<typename T>
	void Append(std::vector<T>& out, const std::vector<T>& more) {
		out.reserve(out.size() + more.size());
		for (auto& i: more)
			out.push_back(i);
	}

	inline void Append(std::vector<unsigned char>& Array, const unsigned char* more, size_t moreSize) {
		auto sz = Array.size();
		Array.resize(sz + moreSize);
		memcpy(Array.data() + sz, more, moreSize);
	}

	template<typename T>
	inline auto Extend(std::vector<T>& vec, size_t more) {
		auto sz = vec.size();
		vec.resize(sz + more);
		return sz;
	}

	template<typename KeyType, typename ValueType>
	inline ValueType GetOrDefault(const std::map<KeyType, ValueType>& Map, const KeyType& key, const ValueType& def) {
		if (auto f = Map.find(key); f != Map.end()) return f->second;
		return def;
	}

	template<typename KeyType, typename ValueType>
	inline ValueType GetOrDefault(const std::unordered_map<KeyType, ValueType>& Map, const KeyType& key, const ValueType& def) {
		if (auto f = Map.find(key); f != Map.end()) return f->second;
		return def;
	}

	inline std::vector<uint8_t> Interleave(const std::vector<std::pair<void*, int>>& Items, size_t Count) {
		int OutStride = PVX::Reduce(Items, 0, [](auto acc, const auto& it) { 
			return acc + it.second; 
		});
		std::vector<uint8_t> ret(OutStride * Count);
		int offset = 0;
		for (auto& [ptr, Stride] : Items) {
			Interleave(ret.data() + offset, OutStride, ptr, Stride, Count);
			offset += Stride;
		}
		return ret;
	}

	template<typename TItem, typename TKey, typename TValue>
	std::unordered_map<TKey, TValue> ToDictionary(const std::vector<TItem>& a, std::function<TKey(const TItem&)> kFnc, std::function<TValue(const TItem&)> vFnc) {
		std::unordered_map<TKey, TValue> ret;
		for (const auto& it : a) {
			ret[kFnc(it)] = vFnc(it);
		}
		return ret;
	}

	template<typename TItem, typename TKey>
	std::unordered_map<TKey, TItem> ToDictionary(const std::vector<TItem>& a, std::function<TKey(const TItem&)> kFnc) {
		std::unordered_map<TKey, TItem> ret;
		for (const auto& it : a) {
			ret[kFnc(it)] = it;
		}
		return ret;
	}
}