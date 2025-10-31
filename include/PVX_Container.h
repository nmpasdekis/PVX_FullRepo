#pragma once
#include <type_traits>
#include <utility>
#include <string>
#include <iterator>
#include <functional>

namespace {
	template <typename, typename = void>
	struct is_container_like: std::false_type {};

	template <typename T>
	struct is_container_like<T, std::void_t<typename T::value_type, typename T::iterator, decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>>: std::true_type {};

	template <typename CharT, typename Traits, typename Alloc>
	struct is_container_like<std::basic_string<CharT, Traits, Alloc>>: std::true_type {};

	template <typename T>
	inline constexpr bool is_container_like_v = is_container_like<T>::value;

	template<typename U, typename = void>
	struct has_size: std::false_type {};

	template<typename U>
	struct has_size<U, std::void_t<decltype(std::declval<U>().size())>>: std::true_type {};

	template <typename T>
	struct container_rebinder;

	// Generic version: works for most STL containers like vector, list, deque, etc.
	template <
		template <class, class...> class Container,
		class OldType, class Alloc, class... Args>
	struct container_rebinder<Container<OldType, Alloc, Args...>> {
		template <class NewType>
		using type = Container<
			NewType,
			typename std::allocator_traits<Alloc>::template rebind_alloc<NewType>,
			Args...>;
	};

	// Specialization for std::basic_string
	template <typename CharT, typename Traits, typename Alloc>
	struct container_rebinder<std::basic_string<CharT, Traits, Alloc>> {
		template <class NewType>
		using type = std::basic_string<
			NewType,
			Traits,
			typename std::allocator_traits<Alloc>::template rebind_alloc<NewType>>;
	};
}

namespace PVX {

	template <typename T, bool Own, typename = std::enable_if_t<is_container_like_v<T>>>
	class _Container {
		using element_t = typename T::value_type;

		// Compile-time ownership
		std::conditional_t<Own, T, T&> data;

		explicit _Container(T& c, std::false_type) noexcept: data(c) {}
		explicit _Container(T&& c, std::true_type) noexcept: data(std::move(c)) {}

		template <typename U>
		friend auto helper(U& c) noexcept;
		template <typename U>
		friend auto helper(U&& c) noexcept;

		template<typename X, bool Y, typename Z>
		friend class _Container;

	public:
		_Container(const _Container&) = delete;
		_Container(_Container&&) = delete;
		_Container& operator=(const _Container&) = delete;
		_Container& operator=(_Container&&) = delete;

		constexpr T& operator()() noexcept {
			if constexpr (Own) return data;
			else return data;
		}
		constexpr const T& operator()() const noexcept {
			if constexpr (Own) return data;
			else return data;
		}

		operator T() && {
			if constexpr (Own) return std::move(data);
			else return data;
		}

		template <typename F>
		void forEach(F&& clb) && {
			for (auto& e : (*this)()) clb(e);
		}

		template <typename F, typename U = T, std::enable_if_t<has_size<U>::value, int> = 0>
		auto map(F&& clb) && {
			using result_t = std::decay_t<decltype(clb(*data.begin()))>;
			using same_container_t = typename container_rebinder<T>::template type<result_t>;
			same_container_t ret;
			if constexpr (has_size<U>::value) ret.reserve(data.size());
			for (const auto& e : (*this)()) ret.emplace_back(clb(e));
			return _Container<same_container_t, true>(std::move(ret), std::true_type{});
		}

		template<typename string_t>
		auto split(const string_t& sep) {
			std::vector<string_t, std::allocator<string_t>> ret;
			auto sepSize = sep.length();
			size_t start = 0;
			size_t next;
			
			while ((next = data.find(sep.c_str(), start)) != data.npos) {
				ret.emplace_back(data.substr(start, next - start));
				start = next + sepSize;
			}
			ret.emplace_back(data.substr(start, data.length() - start));
			return _Container<std::vector<string_t, std::allocator<string_t>>, true>(std::move(ret), std::true_type{});
		}

		template<typename Char_t>
		auto split(const Char_t* sep) {
			using string_t = std::basic_string<Char_t>;
			std::vector<string_t, std::allocator<string_t>> ret;
			auto sepSize = 0;
			while (sep[sepSize]) sepSize++;
			size_t start = 0;
			size_t next;

			while ((next = data.find(sep, start)) != data.npos) {
				ret.emplace_back(data.substr(start, next - start));
				start = next + sepSize;
			}
			ret.emplace_back(data.substr(start, data.length() - start));
			return _Container<std::vector<string_t, std::allocator<string_t>>, true>(std::move(ret), std::true_type{});
		}
	};

	template <typename U>
	auto helper(U& c) noexcept { return _Container<U, false>(c, std::false_type{}); }
	template<typename U>
	auto helper(U&& c) noexcept { return _Container<U, true>(std::move(c), std::true_type{}); }
};