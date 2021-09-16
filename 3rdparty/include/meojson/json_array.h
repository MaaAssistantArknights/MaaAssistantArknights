#pragma once

#include <string>
#include <vector>
#include <initializer_list>

namespace json
{
    class value;

    class array
    {
    public:
        using raw_array = std::vector<value>;
        using iterator = raw_array::iterator;
        using const_iterator = raw_array::const_iterator;
        using reverse_iterator = raw_array::reverse_iterator;
        using const_reverse_iterator = raw_array::const_reverse_iterator;

        array() = default;
        array(const array& rhs) = default;
        array(array&& rhs) noexcept = default;
        array(const raw_array& arr);
        array(raw_array&& arr) noexcept;
        array(std::initializer_list<raw_array::value_type> init_list);
        template<typename ArrayType>
        array(ArrayType arr) {
            static_assert(
                std::is_constructible<json::value, typename ArrayType::value_type>::value,
                "Parameter can't be used to construct a json::value");
            for (auto&& ele : arr) {
                _array_data.emplace_back(std::move(ele));
            }
        }

        ~array() noexcept = default;

        bool empty() const noexcept { return _array_data.empty(); }
        size_t size() const noexcept { return _array_data.size(); }
        bool exist(size_t pos) const { return _array_data.size() < pos; }
        const value& at(size_t pos) const;
        const std::string to_string() const;
        const std::string format(std::string shift_str = "    ", size_t basic_shift_count = 0) const;

        const bool get(size_t pos, bool default_value) const;
        const int get(size_t pos, int default_value) const;
        const long get(size_t pos, long default_value) const;
        const unsigned long get(size_t pos, unsigned default_value) const;
        const long long get(size_t pos, long long default_value) const;
        const unsigned long long get(size_t pos, unsigned long long default_value) const;
        const float get(size_t pos, float default_value) const;
        const double get(size_t pos, double default_value) const;
        const long double get(size_t pos, long double default_value) const;
        const std::string get(size_t pos, std::string default_value) const;
        const std::string get(size_t pos, const char* default_value) const;

        template <typename... Args>
        decltype(auto) emplace_back(Args &&... args)
        {
            static_assert(
                std::is_constructible<raw_array::value_type, Args...>::value,
                "Parameter can't be used to construct a raw_array::value_type");
            return _array_data.emplace_back(std::forward<Args>(args)...);
        }

        void clear() noexcept;
        // void earse(size_t pos);

        iterator begin() noexcept;
        iterator end() noexcept;
        const_iterator begin() const noexcept;
        const_iterator end() const noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;
        reverse_iterator rend() noexcept;
        const_reverse_iterator rbegin() const noexcept;
        const_reverse_iterator rend() const noexcept;
        const_reverse_iterator crbegin() const noexcept;
        const_reverse_iterator crend() const noexcept;

        const value& operator[](size_t pos) const;
        value& operator[](size_t pos);

        array& operator=(const array&) = default;
        array& operator=(array&&) noexcept = default;

        // const raw_array &raw_data() const;

    private:
        raw_array _array_data;
    };

    std::ostream& operator<<(std::ostream& out, const array& arr);

} // namespace json