#pragma once

#include <string>
#include <ostream>
// #include <iostream>
#include <memory>

namespace json
{
    enum class value_type : char
    {
        Invalid,
        Null,
        Boolean,
        String,
        Number,
        Array,
        Object,
        NUM_T
    };

    class array;
    class object;

    class value
    {
        using unique_array = std::unique_ptr<array>;
        using unique_object = std::unique_ptr<object>;

    public:
        value();
        value(const value& rhs);
        value(value&& rhs) noexcept;

        value(bool b);

        value(int num);
        value(unsigned num);
        value(long num);
        value(unsigned long num);
        value(long long num);
        value(unsigned long long num);
        value(float num);
        value(double num);
        value(long double num);

        value(const char* str);
        value(const std::string& str);
        value(std::string&& str);

        value(const array& arr);
        value(array&& arr);
        // value(std::initializer_list<value> init_list); // for array

        value(const object& obj);
        value(object&& obj);
        // error: conversion from ‘<brace-enclosed initializer list>’ to ‘json::value’ is ambiguous
        // value(std::initializer_list<std::pair<std::string, value>> init_list); // for object

        // Constructed from raw data
        template <typename... Args>
        value(value_type type, Args &&...args)
            : _type(type),
            _raw_data(std::forward<Args>(args)...)
        {
            static_assert(
                std::is_constructible<std::string, Args...>::value,
                "Parameter can't be used to construct a std::string");
        }

        // Prohibit conversion of other types to value
        template <typename T>
        value(T) = delete;

        ~value();

        bool valid() const noexcept { return _type != value_type::Invalid ? true : false; }
        bool empty() const noexcept { return (_type == value_type::Null && _raw_data.compare("null") == 0) ? true : false; }
        bool is_null() const noexcept { return empty(); }
        bool is_number() const noexcept { return _type == value_type::Number; }
        bool is_boolean() const noexcept { return _type == value_type::Boolean; }
        bool is_string() const noexcept { return _type == value_type::String; }
        bool is_array() const noexcept { return _type == value_type::Array; }
        bool is_object() const noexcept { return _type == value_type::Object; }
        bool exist(const std::string& key) const;
        bool exist(size_t pos) const;
        value_type type() const noexcept { return _type; }
        const value& at(size_t pos) const;
        const value& at(const std::string& key) const;

        template <typename Type>
        decltype(auto) get(const std::string& key, Type default_value) const
        {
            return is_object() ? as_object().get(key, default_value) : default_value;
        }
        template <typename Type>
        decltype(auto) get(size_t pos, Type default_value) const
        {
            return is_array() ? as_array().get(pos, default_value) : default_value;
        }

        const bool as_boolean() const;
        const int as_integer() const;
        // const unsigned as_unsigned() const;
        const long as_long() const;
        const unsigned long as_unsigned_long() const;
        const long long as_long_long() const;
        const unsigned long long as_unsigned_long_long() const;
        const float as_float() const;
        const double as_double() const;
        const long double as_long_double() const;
        const std::string as_string() const;
        const array& as_array() const;
        const object& as_object() const;

        array& as_array();
        object& as_object();

        // return raw string
        const std::string to_string() const;
        const std::string format(std::string shift_str = "    ", size_t basic_shift_count = 0) const;

        value& operator=(const value& rhs);
        value& operator=(value&&) noexcept;

        const value& operator[](size_t pos) const;
        value& operator[](size_t pos);
        value& operator[](const std::string& key);
        value& operator[](std::string&& key);
        //explicit operator bool() const noexcept { return valid(); }

        explicit operator bool() const { return as_boolean(); }
        explicit operator int() const { return as_integer(); }
        explicit operator long() const { return as_long(); }
        explicit operator unsigned long() const { return as_unsigned_long(); }
        explicit operator long long() const { return as_long_long(); }
        explicit operator unsigned long long() const { return as_unsigned_long_long(); }
        explicit operator float() const { return as_float(); }
        explicit operator double() const { return as_double(); }
        explicit operator long double() const { return as_long_double(); }
        explicit operator std::string() const { return as_string(); }

    private:
        template <typename T>
        static std::unique_ptr<T> copy_unique_ptr(const std::unique_ptr<T>& t)
        {
            return t == nullptr ? nullptr : std::make_unique<T>(*t);
        }

        value_type _type = value_type::Null;
        std::string _raw_data = "null"; // If the value_type is Object or Array, the _raw_data will be a empty string.
        unique_array _array_ptr;
        unique_object _object_ptr;
    };

    const value invalid_value();
    std::ostream& operator<<(std::ostream& out, const value& val);
    // std::istream &operator>>(std::istream &in, value &val);

} // namespace json