#pragma once

#include <type_traits>
#include <utility>
#include <string>
#include <iterator>
#include <optional>


namespace {
    // thank You ChatGPT
    template <typename, typename = void>
    struct is_container_like: std::false_type {};

    template <typename T>
    struct is_container_like<T, std::void_t<typename T::value_type, typename T::iterator, decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>>: std::true_type {};

    template <typename CharT, typename Traits, typename Alloc>
    struct is_container_like<std::basic_string<CharT, Traits, Alloc>>: std::true_type {};

    template <typename T>
    inline constexpr bool is_container_like_v = is_container_like<T>::value;
}


namespace PVX {
    template <typename T, typename = std::enable_if_t<is_container_like_v<T>>>
    class _Container {
        std::optional<T> owned;
        T& ref;

        explicit _Container(T& c) noexcept: ref(c) {}
        explicit _Container(T&& c) noexcept: owned(std::move(c)), ref(*owned) {}
        friend auto make_container(T& c) noexcept { return _Container(c); }
        friend auto make_container(T&& c) noexcept { return _Container(std::move(c)); }
    public:
        using element_t = typename T::value_type;

        auto& get() noexcept { return ref; }
        const auto& get() const noexcept { return ref; }
    };
}