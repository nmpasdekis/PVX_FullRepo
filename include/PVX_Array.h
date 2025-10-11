#pragma once
#include <vector>
#include <functional>
#include <algorithm>
#include <unordered_map>

namespace PVX{
    template<typename T>
    class Array {
    private:
        std::vector<T> Data;

        struct Identity {
            template<typename U>
            constexpr U&& operator()(U&& u) const noexcept {
                return std::forward<U>(u);
            }
        };

    public:
        // --- Constructors ---
        Array() = default;
        Array(std::initializer_list<T> list) : Data(list) {}
        explicit Array(std::vector<T> vec) : Data(std::move(vec)) {}

        bool isEmpty() const { return Data.empty(); }
        void clear() { Data.clear(); }

        template<typename Iter>
        Array(Iter begin, Iter end) {
            Data.assign(begin, end);
        }

        // --- Basic Modifiers ---
        T& push(const T& value) {
            Data.push_back(value);
            return Data.back();
        }

        T& push(T&& value) {
            Data.push_back(std::move(value));
            return Data.back();
        }
        T pop() {
            T value = std::move(Data.back());
            Data.pop_back();
            return value;
        }
        T remove(size_t index) {
            auto it = Data.begin() + index;
            T value = std::move(*it);
            Data.erase(it);
            return value;
        }

        // --- Basic Accessors ---
        T& operator[](size_t index) { return Data[index]; }
        const T& operator[](size_t index) const { return Data[index]; }
        size_t size() const { return Data.size(); }
        bool empty() const { return Data.empty(); }
        auto begin() { return Data.begin(); }
        auto end() { return Data.end(); }
        auto begin() const { return Data.begin(); }
        auto end() const { return Data.end(); }

        // --- map ---
        template<typename Func>
        auto map(Func fnc) const -> Array<decltype(fnc(std::declval<T>()))> {
            using U = decltype(fnc(std::declval<T>()));
            Array<U> ret;
            ret.Data.reserve(Data.size());
            for (const auto& v : Data)
                ret.push(fnc(v));
            return ret;
        }

        template<typename Func>
        auto mapWithIndex(Func fnc) const->Array<decltype(fnc(std::declval<T>(), int{})) > {
            using U = decltype(fnc(std::declval<T>(), int{}));
            Array<U> ret;
            ret.Data.reserve(Data.size());
            int idx = 0;
            for (const auto& v : Data)
                ret.push(fnc(v, idx++));
            return ret;
        }

        // --- forEach ---
        template<typename Func>
        void forEach(Func fnc) {
            for (auto& v : Data)
                fnc(v);
        }

        template<typename Func>
        void forEachWithIndex(Func fnc) const {
            int idx = 0;
            for (const auto& v : Data)
                fnc(v, idx++);
        }

        // --- filter ---
        template<typename Predicate>
        Array<T> filter(Predicate pred) const {
            Array<T> ret;
            for (const auto& v : Data)
                if (pred(v))
                    ret.push(v);
            return ret;
        }

        // --- any / all ---
        template<typename Predicate>
        bool any(Predicate pred) const {
            for (const auto& v : Data)
                if (pred(v))
                    return true;
            return false;
        }

        template<typename Predicate>
        bool all(Predicate pred) const {
            for (const auto& v : Data)
                if (!pred(v))
                    return false;
            return true;
        }

        // --- findFirst ---
        template<typename Predicate>
        T* findFirst(Predicate pred) {
            for (auto& v : Data)
                if (pred(v))
                    return &v;
            return nullptr;
        }

        // --- slice ---
        Array<T> slice(int start, int end) const {
            Array<T> ret;
            int size = static_cast<int>(Data.size());

            if (start < 0) start += size;
            if (end < 0) end += size;
            start = std::max(0, start);
            end = std::min(size, end);

            if (start >= end) return ret;

            for (int i = start; i < end; ++i) {
                ret.push(Data[i]);
            }
            return ret;
        }

        Array<T> slice(int start) const {
            return slice(start, static_cast<int>(Data.size()));
        }

        // --- orderBy (selector) ---
        template<typename Selector>
        Array<T> orderBy(Selector selector) const {
            Array<T> ret = *this;
            std::sort(ret.Data.begin(), ret.Data.end(), [&](const T& a, const T& b) {
                return selector(a) < selector(b);
            });
            return ret;
        }

        // --- min / max / sum (with selector) ---
        template<typename Selector = Identity>
        auto min(Selector selector = Selector{}) const {
            return *std::min_element(Data.begin(), Data.end(), [&](const T& a, const T& b) {
                return selector(a) < selector(b);
            });
        }

        template<typename Selector = Identity>
        auto max(Selector selector = Selector{}) const {
            return *std::max_element(Data.begin(), Data.end(), [&](const T& a, const T& b) {
                return selector(a) < selector(b);
             });
        }

        template<typename Selector = Identity>
        auto sum(Selector selector = Identity{}) const {
            using ResultType = decltype(selector(std::declval<T>()));
            ResultType total{};
            for (const auto& v : Data) total += selector(v);
            return total;
        }

        // --- toDictionary ---
        template<typename KeySelector>
        auto toDictionary(KeySelector keySelector) const {
            std::unordered_map<decltype(keySelector(std::declval<T>())), T> ret;
            for (const auto& v : Data)
                ret[keySelector(v)] = v;
            return ret;
        }

        template<typename KeySelector, typename ValueSelector>
        auto toDictionary(KeySelector keySelector, ValueSelector valueSelector) const {
            std::unordered_map<decltype(keySelector(std::declval<T>())), decltype(valueSelector(std::declval<T>()))> ret;
            for (const auto& v : Data)
                ret[keySelector(v)] = valueSelector(v);
            return ret;
        }

        // --- groupBy ---
        template<typename KeySelector>
        auto groupBy(KeySelector keySelector) const {
            using KeyType = decltype(keySelector(std::declval<T>()));
            std::unordered_map<KeyType, Array<T>> groups;
            for (const auto& v : Data)
                groups[keySelector(v)].push(v);
            return groups;
        }

        template<typename KeySelector, typename ValueSelector>
        auto groupBy(KeySelector keySelector, ValueSelector valueSelector) const {
            using KeyType = decltype(keySelector(std::declval<T>()));
            using ValueType = decltype(valueSelector(std::declval<T>()));
            std::unordered_map<KeyType, Array<ValueType>> groups;
            for (const auto& v : Data)
                groups[keySelector(v)].push(valueSelector(v));
            return groups;
        }
    };
}