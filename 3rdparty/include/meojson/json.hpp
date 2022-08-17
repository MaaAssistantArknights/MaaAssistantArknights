#pragma once

#include <initializer_list>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <variant>
#include <string_view>
#include <algorithm>
#include <fstream>
#include <sstream>

#define MEOJSON_INLINE inline

namespace json
{
    class array;
    class object;

    // *************************
    // *     value declare     *
    // *************************

    class value
    {
        using array_ptr = std::unique_ptr<array>;
        using object_ptr = std::unique_ptr<object>;
    public:
        enum class value_type : char
        {
            Invalid,
            Null,
            Boolean,
            String,
            Number,
            Array,
            Object
        };

        using var_t = std::variant<std::string, array_ptr, object_ptr>;

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
        value(std::string str);

        value(array arr);
        // value(std::initializer_list<value> init_list); // for array

        value(object obj);
        // error: conversion from ‘<brace-enclosed initializer list>’ to ‘value’
        // is ambiguous value(std::initializer_list<std::pair<std::string, value>>
        // init_list); // for object

        // Constructed from raw data
        template <typename... Args> value(value_type type, Args &&...args);

        // Prohibit conversion of other types to value
        template <typename T> value(T) = delete;

        ~value();

        bool valid() const noexcept { return _type != value_type::Invalid; }
        bool empty() const noexcept { return is_null(); }
        bool is_null() const noexcept { return _type == value_type::Null; }
        bool is_number() const noexcept { return _type == value_type::Number; }
        bool is_boolean() const noexcept { return _type == value_type::Boolean; }
        bool is_string() const noexcept { return _type == value_type::String; }
        bool is_array() const noexcept { return _type == value_type::Array; }
        bool is_object() const noexcept { return _type == value_type::Object; }
        bool contains(const std::string& key) const;
        bool contains(size_t pos) const;
        bool exists(const std::string& key) const { return contains(key); }
        bool exists(size_t pos) const { return contains(pos); }
        value_type type() const noexcept { return _type; }
        const value& at(size_t pos) const;
        const value& at(const std::string& key) const;

        // usage: get(key, key_child, ..., default_value);
        template <typename... KeysThenDefaultValue>
        decltype(auto) get(KeysThenDefaultValue &&... keys_then_default_value) const;

        template <typename Type = value>
        std::optional<Type> find(size_t pos) const;
        template <typename Type = value>
        std::optional<Type> find(const std::string& key) const;

        bool as_boolean() const;
        int as_integer() const;
        // unsigned as_unsigned() const;
        long as_long() const;
        unsigned long as_unsigned_long() const;
        long long as_long_long() const;
        unsigned long long as_unsigned_long_long() const;
        float as_float() const;
        double as_double() const;
        long double as_long_double() const;
        const std::string as_string() const;
        const array& as_array() const;
        const object& as_object() const;

        array& as_array();
        object& as_object();

        template<typename... Args> decltype(auto) array_emplace(Args &&...args);
        template<typename... Args> decltype(auto) object_emplace(Args &&...args);
        void clear() noexcept;

        // return raw string
        const std::string to_string() const;
        const std::string format(bool ordered = false,
            std::string shift_str = "    ", size_t basic_shift_count = 0) const;

        value& operator=(const value& rhs);
        value& operator=(value&&) noexcept;

        const value& operator[](size_t pos) const;
        value& operator[](size_t pos);
        value& operator[](const std::string& key);
        value& operator[](std::string&& key);

        value operator|(const object& rhs)&;
        value operator|(object&& rhs)&;
        value operator|(const object& rhs)&&;
        value operator|(object&& rhs)&&;

        value& operator|=(const object& rhs);
        value& operator|=(object&& rhs);

        //value operator&(const object& rhs)&;
        //value operator&(object&& rhs)&;
        //value operator&(const object& rhs)&&;
        //value operator&(object&& rhs)&&;

        //value& operator&=(const object& rhs);
        //value& operator&=(object&& rhs);

        value operator+(const array& rhs)&;
        value operator+(array&& rhs)&;
        value operator+(const array& rhs)&&;
        value operator+(array&& rhs)&&;

        value& operator+=(const array& rhs);
        value& operator+=(array&& rhs);

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
        static var_t deep_copy(const var_t& src);

        template <typename... KeysThenDefaultValue, size_t... KeysIndexes>
        decltype(auto) get(
            std::tuple<KeysThenDefaultValue...> keys_then_default_value,
            std::index_sequence<KeysIndexes...>) const;

        template <typename T, typename FirstKey, typename... RestKeys>
        decltype(auto) get_aux(T&& default_value, FirstKey&& first, RestKeys &&... rest) const;
        template <typename T, typename UniqueKey>
        decltype(auto) get_aux(T&& default_value, UniqueKey&& first) const;

        const std::string& as_basic_type_str() const;
        std::string& as_basic_type_str();

        value_type _type = value_type::Null;
        var_t _raw_data;
    };

    const value invalid_value();
    std::ostream& operator<<(std::ostream& out, const value& val);

    // *************************
    // *     array declare     *
    // *************************
    class array
    {
    public:
        using raw_array = std::vector<value>;
        using value_type = raw_array::value_type;
        using iterator = raw_array::iterator;
        using const_iterator = raw_array::const_iterator;
        using reverse_iterator = raw_array::reverse_iterator;
        using const_reverse_iterator = raw_array::const_reverse_iterator;

    public:
        array() = default;
        array(const array& rhs) = default;
        array(array&& rhs) noexcept = default;
        array(const raw_array& arr);
        array(raw_array&& arr) noexcept;
        array(std::initializer_list<raw_array::value_type> init_list);
        array(raw_array::size_type size);

        explicit array(const value& val);
        explicit array(value&& val);
        template<typename ArrayType, typename EnableT = std::enable_if_t<
            std::is_constructible_v<value, typename ArrayType::value_type>>>
            array(ArrayType arr);

        ~array() noexcept = default;

        bool empty() const noexcept { return _array_data.empty(); }
        size_t size() const noexcept { return _array_data.size(); }
        bool contains(size_t pos) const { return pos < _array_data.size(); }
        bool exists(size_t pos) const { return contains(pos); }
        const value& at(size_t pos) const;
        const std::string to_string() const;
        const std::string format(bool ordered = false,
            std::string shift_str = "    ", size_t basic_shift_count = 0) const;

        bool get(size_t pos, bool default_value) const;
        int get(size_t pos, int default_value) const;
        long get(size_t pos, long default_value) const;
        unsigned long get(size_t pos, unsigned default_value) const;
        long long get(size_t pos, long long default_value) const;
        unsigned long long get(size_t pos,
                                     unsigned long long default_value) const;
        float get(size_t pos, float default_value) const;
        double get(size_t pos, double default_value) const;
        long double get(size_t pos, long double default_value) const;
        const std::string get(size_t pos, std::string default_value) const;
        const std::string get(size_t pos, const char* default_value) const;
        const value& get(size_t pos) const;

        template <typename Type = value>
        std::optional<Type> find(size_t pos) const;

        template <typename... Args> decltype(auto) emplace_back(Args &&...args);

        void clear() noexcept;
        // void erase(size_t pos);

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

        array operator+(const array& rhs)&;
        array operator+(array&& rhs)&;
        array operator+(const array& rhs)&&;
        array operator+(array&& rhs)&&;

        array& operator+=(const array& rhs);
        array& operator+=(array&& rhs);

        array& operator=(const array&) = default;
        array& operator=(array&&) noexcept = default;

        // const raw_array &raw_data() const;

    private:
        raw_array _array_data;
    };

    std::ostream& operator<<(std::ostream& out, const array& arr);

    // *************************
    // *     object declare    *
    // *************************
    class object
    {
    public:
        using raw_object = std::unordered_map<std::string, value>;
        using value_type = raw_object::value_type;
        using iterator = raw_object::iterator;
        using const_iterator = raw_object::const_iterator;

    public:
        object() = default;
        object(const object& rhs) = default;
        object(object&& rhs) noexcept = default;
        object(const raw_object& raw_obj);
        object(raw_object&& raw_obj);
        object(std::initializer_list<raw_object::value_type> init_list);
        explicit object(const value& val);
        explicit object(value&& val);
        template <typename MapType, typename EnableT = std::enable_if_t<
            std::is_constructible_v<raw_object::value_type, typename MapType::value_type>>>
            object(MapType map);

        ~object() = default;

        bool empty() const noexcept { return _object_data.empty(); }
        size_t size() const noexcept { return _object_data.size(); }
        bool contains(const std::string& key) const { return _object_data.find(key) != _object_data.cend(); }
        bool exists(const std::string& key) const { return contains(key); }
        const value& at(const std::string& key) const;
        const std::string to_string() const;
        const std::string format(bool ordered = false,
            std::string shift_str = "    ", size_t basic_shift_count = 0) const;

        bool get(const std::string& key, bool default_value) const;
        int get(const std::string& key, int default_value) const;
        long get(const std::string& key, long default_value) const;
        unsigned long get(const std::string& key, unsigned default_value) const;
        long long get(const std::string& key, long long default_value) const;
        unsigned long long get(const std::string& key,
                                     unsigned long long default_value) const;
        float get(const std::string& key, float default_value) const;
        double get(const std::string& key, double default_value) const;
        long double get(const std::string& key,
                              long double default_value) const;
        const std::string get(const std::string& key,
                              std::string default_value) const;
        const std::string get(const std::string& key,
                              const char* default_value) const;
        const value& get(const std::string& key) const;

        template <typename Type = value>
        std::optional<Type> find(const std::string& key) const;

        template <typename... Args> decltype(auto) emplace(Args &&...args);
        template <typename... Args> decltype(auto) insert(Args &&...args);

        void clear() noexcept;
        bool erase(const std::string& key);

        iterator begin() noexcept;
        iterator end() noexcept;
        const_iterator begin() const noexcept;
        const_iterator end() const noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        value& operator[](const std::string& key);
        value& operator[](std::string&& key);

        object operator|(const object& rhs)&;
        object operator|(object&& rhs)&;
        object operator|(const object& rhs)&&;
        object operator|(object&& rhs)&&;

        object& operator|=(const object& rhs);
        object& operator|=(object&& rhs);

        //object operator&(const object& rhs)&;
        //object operator&(object&& rhs)&;
        //object operator&(const object& rhs)&&;
        //object operator&(object&& rhs)&&;

        //object& operator&=(const object& rhs);
        //object& operator&=(object&& rhs);

        object& operator=(const object&) = default;
        object& operator=(object&&) = default;

        // const raw_object &raw_data() const;

    private:
        raw_object _object_data;
    };

    std::ostream& operator<<(std::ostream& out, const object& obj);

    // *************************
    // *   exception declare   *
    // *************************
    class exception : public std::exception
    {
    public:
        exception() = default;
        exception(const std::string& msg) : _what(msg) {}

        exception(const exception&) = default;
        exception& operator=(const exception&) = default;
        exception(exception&&) = default;
        exception& operator=(exception&&) = default;

        virtual ~exception() noexcept override = default;

        virtual const char* what() const noexcept override
        {
            return _what.empty() ? "Unknown exception" : _what.c_str();
        }

    protected:
        std::string _what;
    };

    // *************************
    // *      aux declare      *
    // *************************
    std::string unescape_string(std::string str);
    std::string escape_string(std::string str);

    // *************************
    // *       value impl      *
    // *************************
    MEOJSON_INLINE value::value() = default;

    MEOJSON_INLINE value::value(const value& rhs)
        : _type(rhs._type), _raw_data(deep_copy(rhs._raw_data))
    {
        ;
    }

    MEOJSON_INLINE value::value(value&& rhs) noexcept = default;

    MEOJSON_INLINE value::value(bool b)
        : _type(value_type::Boolean), _raw_data(std::string(b ? "true" : "false"))
    {
        ;
    }

    MEOJSON_INLINE value::value(int num)
        : _type(value_type::Number), _raw_data(std::to_string(num))
    {
        ;
    }

    MEOJSON_INLINE value::value(unsigned num)
        : _type(value_type::Number), _raw_data(std::to_string(num))
    {
        ;
    }

    MEOJSON_INLINE value::value(long num)
        : _type(value_type::Number), _raw_data(std::to_string(num))
    {
        ;
    }

    MEOJSON_INLINE value::value(unsigned long num)
        : _type(value_type::Number), _raw_data(std::to_string(num))
    {
        ;
    }

    MEOJSON_INLINE value::value(long long num)
        : _type(value_type::Number), _raw_data(std::to_string(num))
    {
        ;
    }

    MEOJSON_INLINE value::value(unsigned long long num)
        : _type(value_type::Number), _raw_data(std::to_string(num))
    {
        ;
    }

    MEOJSON_INLINE value::value(float num)
        : _type(value_type::Number), _raw_data(std::to_string(num))
    {
        ;
    }

    MEOJSON_INLINE value::value(double num)
        : _type(value_type::Number), _raw_data(std::to_string(num))
    {
        ;
    }

    MEOJSON_INLINE value::value(long double num)
        : _type(value_type::Number), _raw_data(std::to_string(num))
    {
        ;
    }

    MEOJSON_INLINE value::value(const char* str)
        : _type(value_type::String), _raw_data(unescape_string(str))
    {
        ;
    }

    MEOJSON_INLINE value::value(std::string str)
        : _type(value_type::String),
        _raw_data(unescape_string(std::move(str)))
    {
        ;
    }

    MEOJSON_INLINE value::value(array arr)
        : _type(value_type::Array),
        _raw_data(std::make_unique<array>(std::move(arr)))
    {
        ;
    }

    MEOJSON_INLINE value::value(object obj)
        : _type(value_type::Object),
        _raw_data(std::make_unique<object>(std::move(obj)))
    {
        ;
    }

    // for Pimpl
    MEOJSON_INLINE value::~value() = default;

    MEOJSON_INLINE bool value::contains(const std::string& key) const
    {
        return is_object() && as_object().contains(key);
    }

    MEOJSON_INLINE bool value::contains(size_t pos) const
    {
        return is_array() && as_array().contains(pos);
    }

    MEOJSON_INLINE const value& value::at(size_t pos) const
    {
        return as_array().at(pos);
    }

    MEOJSON_INLINE const value& value::at(const std::string& key) const
    {
        return as_object().at(key);
    }

    template <typename... KeysThenDefaultValue>
    MEOJSON_INLINE decltype(auto) value::get(
        KeysThenDefaultValue &&... keys_then_default_value) const
    {
        return get(
            std::forward_as_tuple(keys_then_default_value...),
            std::make_index_sequence<sizeof...(keys_then_default_value) - 1>{});
    }

    template <typename... KeysThenDefaultValue, size_t... KeysIndexes>
    MEOJSON_INLINE decltype(auto) value::get(
        std::tuple<KeysThenDefaultValue...> keys_then_default_value,
        std::index_sequence<KeysIndexes...>) const
    {
        constexpr unsigned long DefaultValueIndex = sizeof...(KeysThenDefaultValue) - 1;
        return get_aux(
            std::get<DefaultValueIndex>(keys_then_default_value),
            std::get<KeysIndexes>(keys_then_default_value)...);
    }

    template <typename T, typename FirstKey, typename... RestKeys>
    MEOJSON_INLINE decltype(auto) value::get_aux(T&& default_value, FirstKey&& first, RestKeys &&... rest) const
    {
        if constexpr (std::is_constructible<std::string, FirstKey>::value) {
            return is_object() ?
                as_object().get(
                    std::forward<FirstKey>(first)).get_aux(
                        std::forward<T>(default_value), std::forward<RestKeys>(rest)...)
                : default_value;
        }
        else if constexpr (std::is_integral<typename std::remove_reference<FirstKey>::type>::value) {
            return is_array()
                ? as_array().get(
                    std::forward<FirstKey>(first)).get_aux(
                        std::forward<T>(default_value), std::forward<RestKeys>(rest)...)
                : default_value;
        }
        else {
            static_assert(!sizeof(FirstKey), "Parameter must be integral or std::string constructible");
        }
    }

    template <typename T, typename UniqueKey>
    MEOJSON_INLINE decltype(auto) value::get_aux(T&& default_value, UniqueKey&& first) const
    {
        if constexpr (std::is_constructible<std::string, UniqueKey>::value) {
            return is_object() ?
                as_object().get(std::forward<UniqueKey>(first), std::forward<T>(default_value))
                : default_value;
        }
        else if constexpr (std::is_integral<typename std::remove_reference<UniqueKey>::type>::value) {
            return is_array() ?
                as_array().get(std::forward<UniqueKey>(first), std::forward<T>(default_value))
                : default_value;
        }
        else {
            static_assert(!sizeof(UniqueKey), "Parameter must be integral or std::string constructible");
        }
    }

    template <typename Type>
    MEOJSON_INLINE std::optional<Type> value::find(size_t pos) const
    {
        return is_array() ? as_array().template find<Type>(pos) : std::nullopt;
    }

    template <typename Type>
    MEOJSON_INLINE std::optional<Type> value::find(const std::string& key) const
    {
        return is_object() ? as_object().template find<Type>(key) : std::nullopt;
    }

    MEOJSON_INLINE bool value::as_boolean() const
    {
        if (is_boolean()) {
            if (const std::string& b_str = as_basic_type_str();
                b_str == "true") {
                return true;
            }
            else if (b_str == "false") {
                return false;
            }
            else {
                throw exception("Unknown Parse Error");
            }
        }
        else {
            throw exception("Wrong Type");
        }
    }

    MEOJSON_INLINE int value::as_integer() const
    {
        if (is_number()) {
            return std::stoi(as_basic_type_str());
        }
        else {
            throw exception("Wrong Type");
        }
    }

    // const unsigned value::as_unsigned() const
    // {
    //     if (is_number())
    //     {
    //         return std::stou(_raw_data); // not contains
    //     }
    //     else
    //     {
    //         throw exception("Wrong Type");
    //     }
    // }

    MEOJSON_INLINE long value::as_long() const
    {
        if (is_number()) {
            return std::stol(as_basic_type_str());
        }
        else {
            throw exception("Wrong Type");
        }
    }

    MEOJSON_INLINE unsigned long value::as_unsigned_long() const
    {
        if (is_number()) {
            return std::stoul(as_basic_type_str());
        }
        else {
            throw exception("Wrong Type");
        }
    }

    MEOJSON_INLINE long long value::as_long_long() const
    {
        if (is_number()) {
            return std::stoll(as_basic_type_str());
        }
        else {
            throw exception("Wrong Type");
        }
    }

    MEOJSON_INLINE unsigned long long value::as_unsigned_long_long() const
    {
        if (is_number()) {
            return std::stoull(as_basic_type_str());
        }
        else {
            throw exception("Wrong Type");
        }
    }

    MEOJSON_INLINE float value::as_float() const
    {
        if (is_number()) {
            return std::stof(as_basic_type_str());
        }
        else {
            throw exception("Wrong Type");
        }
    }

    MEOJSON_INLINE double value::as_double() const
    {
        if (is_number()) {
            return std::stod(as_basic_type_str());
        }
        else {
            throw exception("Wrong Type");
        }
    }

    MEOJSON_INLINE long double value::as_long_double() const
    {
        if (is_number()) {
            return std::stold(as_basic_type_str());
        }
        else {
            throw exception("Wrong Type");
        }
    }

    MEOJSON_INLINE const std::string value::as_string() const
    {
        if (is_string()) {
            return escape_string(as_basic_type_str());
        }
        else {
            throw exception("Wrong Type");
        }
    }

    MEOJSON_INLINE const array& value::as_array() const
    {
        if (is_array()) {
            return *std::get<array_ptr>(_raw_data);
        }

        throw exception("Wrong Type");
    }

    MEOJSON_INLINE const object& value::as_object() const
    {
        if (is_object()) {
            return *std::get<object_ptr>(_raw_data);
        }

        throw exception("Wrong Type or data empty");
    }

    MEOJSON_INLINE array& value::as_array()
    {
        if (empty()) {
            *this = array();
        }

        if (is_array()) {
            return *std::get<array_ptr>(_raw_data);
        }

        throw exception("Wrong Type");
    }

    MEOJSON_INLINE object& value::as_object()
    {
        if (empty()) {
            *this = object();
        }

        if (is_object()) {
            return *std::get<object_ptr>(_raw_data);
        }

        throw exception("Wrong Type or data empty");
    }

    MEOJSON_INLINE const std::string& value::as_basic_type_str() const
    {
        return std::get<std::string>(_raw_data);
    }
    MEOJSON_INLINE std::string& value::as_basic_type_str()
    {
        return std::get<std::string>(_raw_data);
    }

    template<typename... Args>
    MEOJSON_INLINE decltype(auto) value::array_emplace(Args &&...args)
    {
        return as_array().emplace_back(std::forward<Args>(args)...);
    }

    template<typename... Args>
    MEOJSON_INLINE decltype(auto) value::object_emplace(Args &&...args)
    {
        return as_object().emplace(std::forward<Args>(args)...);
    }

    MEOJSON_INLINE void value::clear() noexcept
    {
        *this = value();
    }

    MEOJSON_INLINE const std::string value::to_string() const
    {
        switch (_type) {
        case value_type::Null:
            return "null";
        case value_type::Boolean:
        case value_type::Number:
            return as_basic_type_str();
        case value_type::String:
            return '"' + as_basic_type_str() + '"';
        case value_type::Array:
            return as_array().to_string();
        case value_type::Object:
            return as_object().to_string();
        default:
            throw exception("Unknown Value Type");
        }
    }

    MEOJSON_INLINE const std::string value::format(bool ordered,
        std::string shift_str, size_t basic_shift_count) const
    {
        switch (_type) {
        case value_type::Null:
            return "null";
        case value_type::Boolean:
        case value_type::Number:
            return as_basic_type_str();
        case value_type::String:
            return '"' + as_basic_type_str() + '"';
        case value_type::Array:
            return as_array().format(ordered, shift_str, basic_shift_count);
        case value_type::Object:
            return as_object().format(ordered, shift_str, basic_shift_count);
        default:
            throw exception("Unknown Value Type");
        }
    }

    MEOJSON_INLINE value& value::operator=(const value& rhs)
    {
        _type = rhs._type;
        _raw_data = deep_copy(rhs._raw_data);

        return *this;
    }

    MEOJSON_INLINE value& value::operator=(value&& rhs) noexcept = default;

    MEOJSON_INLINE const value& value::operator[](size_t pos) const
    {
        // Array not support to create by operator[]

        return as_array()[pos];
    }

    MEOJSON_INLINE value& value::operator[](size_t pos)
    {
        // Array not support to create by operator[]

        return as_array()[pos];
    }

    MEOJSON_INLINE value& value::operator[](const std::string& key)
    {
        if (empty()) {
            *this = object();
        }

        return as_object()[key];
    }

    MEOJSON_INLINE value& value::operator[](std::string&& key)
    {
        if (empty()) {
            *this = object();
        }

        return as_object()[std::move(key)];
    }

    MEOJSON_INLINE value value::operator|(const object& rhs)&
    {
        return as_object() | rhs;
    }

    MEOJSON_INLINE value value::operator|(object&& rhs)&
    {
        return as_object() | std::move(rhs);
    }

    MEOJSON_INLINE value value::operator|(const object& rhs)&&
    {
        return std::move(as_object()) | rhs;
    }

    MEOJSON_INLINE value value::operator|(object&& rhs)&&
    {
        return std::move(as_object()) | std::move(rhs);
    }

    MEOJSON_INLINE value& value::operator|=(const object& rhs)
    {
        as_object() |= rhs;
        return *this;
    }

    MEOJSON_INLINE value& value::operator|=(object&& rhs)
    {
        as_object() |= std::move(rhs);
        return *this;
    }

    //MEOJSON_INLINE value value::operator&(const object& rhs)&
    //{
    //    return as_object() & rhs;
    //}

    //MEOJSON_INLINE value value::operator&(object&& rhs)&
    //{
    //    return as_object() & std::move(rhs);
    //}

    //MEOJSON_INLINE value value::operator&(const object& rhs)&&
    //{
    //    return std::move(as_object()) & rhs;
    //}

    //MEOJSON_INLINE value value::operator&(object&& rhs)&&
    //{
    //    return std::move(as_object()) & std::move(rhs);
    //}

    //MEOJSON_INLINE value& value::operator&=(const object& rhs)
    //{
    //    as_object() &= rhs;
    //    return *this;
    //}

    //MEOJSON_INLINE value& value::operator&=(object&& rhs)
    //{
    //    as_object() &= std::move(rhs);
    //    return *this;
    //}

    MEOJSON_INLINE value value::operator+(const array& rhs)&
    {
        return as_array() + rhs;
    }

    MEOJSON_INLINE value value::operator+(array&& rhs)&
    {
        return as_array() + std::move(rhs);
    }

    MEOJSON_INLINE value value::operator+(const array& rhs)&&
    {
        return std::move(as_array()) + rhs;
    }

    MEOJSON_INLINE value value::operator+(array&& rhs)&&
    {
        return std::move(as_array()) + std::move(rhs);
    }

    MEOJSON_INLINE value& value::operator+=(const array& rhs)
    {
        as_array() += rhs;
        return *this;
    }

    MEOJSON_INLINE value& value::operator+=(array&& rhs)
    {
        as_array() += std::move(rhs);
        return *this;
    }

    template <typename... Args>
    value::value(value_type type, Args &&...args)
        : _type(type), _raw_data(std::forward<Args>(args)...)
    {
        static_assert(std::is_constructible<var_t, Args...>::value,
                      "Parameter can't be used to construct a var_t");
    }

    MEOJSON_INLINE value::var_t value::deep_copy(const var_t& src)
    {
        var_t dst;
        if (const auto string_ptr = std::get_if<std::string>(&src)) {
            dst = *string_ptr;
        }
        else if (const auto arr_ptr = std::get_if<array_ptr>(&src)) {
            dst = std::make_unique<array>(**arr_ptr);
        }
        else if (const auto obj_ptr = std::get_if<object_ptr>(&src)) {
            dst = std::make_unique<object>(**obj_ptr);
        }
        else {
            // maybe invalid_value
        }

        return dst;
    }

    // *************************
    // *       array impl      *
    // *************************
    template <typename... Args> decltype(auto) array::emplace_back(Args &&...args)
    {
        static_assert(std::is_constructible<raw_array::value_type, Args...>::value,
                      "Parameter can't be used to construct a raw_array::value_type");
        return _array_data.emplace_back(std::forward<Args>(args)...);
    }

    MEOJSON_INLINE array::array(const raw_array& arr) : _array_data(arr) { ; }

    MEOJSON_INLINE array::array(raw_array&& arr) noexcept
        : _array_data(std::move(arr))
    {
        ;
    }

    MEOJSON_INLINE
        array::array(std::initializer_list<raw_array::value_type> init_list)
        : _array_data(init_list)
    {
        ;
    }

    MEOJSON_INLINE
        array::array(raw_array::size_type size)
        : _array_data(size)
    {
        ;
    }

    MEOJSON_INLINE array::array(const value& val)
        : array(val.as_array())
    {
        ;
    }

    MEOJSON_INLINE array::array(value&& val)
        : array(std::move(val.as_array()))
    {
        ;
    }

    template<typename ArrayType, typename EnableT>
    MEOJSON_INLINE array::array(ArrayType arr)
    {
        _array_data.assign(
            std::make_move_iterator(arr.begin()),
            std::make_move_iterator(arr.end()));
    }

    MEOJSON_INLINE const value& array::at(size_t pos) const
    {
        return _array_data.at(pos);
    }

    MEOJSON_INLINE void array::clear() noexcept { _array_data.clear(); }

    MEOJSON_INLINE const std::string array::to_string() const
    {
        std::string str = "[";
        for (const value& val : _array_data) {
            str += val.to_string() + ",";
        }
        if (str.back() == ',') {
            str.pop_back();
        }
        str += "]";
        return str;
    }

    MEOJSON_INLINE const std::string array::format(bool ordered,
        std::string shift_str, size_t basic_shift_count) const
    {
        std::string shift;
        for (size_t i = 0; i != basic_shift_count + 1; ++i) {
            shift += shift_str;
        }

        std::string str = "[";
        for (const value& val : _array_data) {
            str += "\n" + shift + val.format(ordered, shift_str, basic_shift_count + 1) + ",";
        }
        if (str.back() == ',') {
            str.pop_back(); // pop last ','
        }

        str += '\n';
        for (size_t i = 0; i != basic_shift_count; ++i) {
            str += shift_str;
        }
        str += ']';
        return str;
    }

    MEOJSON_INLINE bool array::get(size_t pos, bool default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_boolean()) {
                return value.as_boolean();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE int array::get(size_t pos, int default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_number()) {
                return value.as_integer();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE long array::get(size_t pos, long default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_number()) {
                return value.as_long();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE unsigned long array::get(size_t pos,
                                                  unsigned default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_number()) {
                return value.as_unsigned_long();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE long long array::get(size_t pos,
                                              long long default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_number()) {
                return value.as_long_long();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE unsigned long long
        array::get(size_t pos, unsigned long long default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_number()) {
                return value.as_unsigned_long_long();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE float array::get(size_t pos, float default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_number()) {
                return value.as_float();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE double array::get(size_t pos, double default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_number()) {
                return value.as_double();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE long double array::get(size_t pos,
                                                long double default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_number()) {
                return value.as_long_double();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE const std::string array::get(size_t pos,
                                                std::string default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_string()) {
                return value.as_string();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE const std::string array::get(size_t pos,
                                                const char* default_value) const
    {
        if (contains(pos)) {
            value value = _array_data.at(pos);
            if (value.is_string()) {
                return value.as_string();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE const value& array::get(size_t pos) const
    {
        if (contains(pos)) {
            return _array_data.at(pos);
        }
        else {
            static value null;
            return null;
        }
    }

    template <typename Type>
    MEOJSON_INLINE std::optional<Type> array::find(size_t pos) const
    {
        static_assert(std::is_constructible<Type, value>::value, "Type can NOT be constructed by value");
        if (!contains(pos)) {
            return std::nullopt;
        }
        return static_cast<Type>(_array_data.at(pos));
    }

    MEOJSON_INLINE array::iterator array::begin() noexcept
    {
        return _array_data.begin();
    }

    MEOJSON_INLINE array::iterator array::end() noexcept
    {
        return _array_data.end();
    }

    MEOJSON_INLINE array::const_iterator array::begin() const noexcept
    {
        return _array_data.begin();
    }

    MEOJSON_INLINE array::const_iterator array::end() const noexcept
    {
        return _array_data.end();
    }

    MEOJSON_INLINE array::const_iterator array::cbegin() const noexcept
    {
        return _array_data.cbegin();
    }

    MEOJSON_INLINE array::const_iterator array::cend() const noexcept
    {
        return _array_data.cend();
    }

    MEOJSON_INLINE array::reverse_iterator array::rbegin() noexcept
    {
        return _array_data.rbegin();
    }

    MEOJSON_INLINE array::reverse_iterator array::rend() noexcept
    {
        return _array_data.rend();
    }

    MEOJSON_INLINE array::const_reverse_iterator array::rbegin() const noexcept
    {
        return _array_data.rbegin();
    }

    MEOJSON_INLINE array::const_reverse_iterator array::rend() const noexcept
    {
        return _array_data.rend();
    }

    MEOJSON_INLINE array::const_reverse_iterator array::crbegin() const noexcept
    {
        return _array_data.crbegin();
    }

    MEOJSON_INLINE array::const_reverse_iterator array::crend() const noexcept
    {
        return _array_data.crend();
    }

    MEOJSON_INLINE value& array::operator[](size_t pos) { return _array_data[pos]; }

    MEOJSON_INLINE const value& array::operator[](size_t pos) const
    {
        return _array_data[pos];
    }

    MEOJSON_INLINE array array::operator+(const array& rhs)&
    {
        array temp = *this;
        temp._array_data.insert(_array_data.end(), rhs.begin(), rhs.end());
        return temp;
    }

    MEOJSON_INLINE array array::operator+(array&& rhs)&
    {
        array temp = *this;
        temp._array_data.insert(_array_data.end(),
            std::make_move_iterator(rhs.begin()),
            std::make_move_iterator(rhs.end()));
        return temp;
    }

    MEOJSON_INLINE array array::operator+(const array& rhs)&&
    {
        _array_data.insert(_array_data.end(), rhs.begin(), rhs.end());
        return std::move(*this);
    }

    MEOJSON_INLINE array array::operator+(array&& rhs)&&
    {
        _array_data.insert(_array_data.end(),
            std::make_move_iterator(rhs.begin()),
            std::make_move_iterator(rhs.end()));
        return std::move(*this);
    }

    MEOJSON_INLINE array& array::operator+=(const array& rhs)
    {
        _array_data.insert(_array_data.end(), rhs.begin(), rhs.end());
        return *this;
    }

    MEOJSON_INLINE array& array::operator+=(array&& rhs)
    {
        _array_data.insert(_array_data.end(),
            std::make_move_iterator(rhs.begin()),
            std::make_move_iterator(rhs.end()));
        return *this;
    }

    // const raw_array &array::raw_data() const
    // {
    //     return _array_data;
    // }

    // *************************
    // *      object impl      *
    // *************************
    template <typename... Args> decltype(auto) object::emplace(Args &&...args)
    {
        static_assert(
            std::is_constructible<raw_object::value_type, Args...>::value,
            "Parameter can't be used to construct a raw_object::value_type");
        return _object_data.emplace(std::forward<Args>(args)...);
    }

    template <typename... Args> decltype(auto) object::insert(Args &&...args)
    {
        return _object_data.insert(std::forward<Args>(args)...);
    }

    MEOJSON_INLINE std::ostream& operator<<(std::ostream& out, const array& arr)
    {
        // TODO: format output

        out << arr.to_string();
        return out;
    }

    MEOJSON_INLINE object::object(const raw_object& raw_obj)
        : _object_data(raw_obj)
    {
        ;
    }

    MEOJSON_INLINE object::object(raw_object&& raw_obj)
        : _object_data(std::move(raw_obj))
    {
        ;
    }

    MEOJSON_INLINE
        object::object(std::initializer_list<raw_object::value_type> init_list)
    {
        _object_data.reserve(init_list.size());
        for (const auto& [key, val] : init_list) {
            emplace(key, val);
        }
    }

    MEOJSON_INLINE object::object(const value& val)
        : object(val.as_object())
    {
        ;
    }

    MEOJSON_INLINE object::object(value&& val)
        : object(std::move(val.as_object()))
    {
        ;
    }

    MEOJSON_INLINE const value& object::at(const std::string& key) const
    {
        return _object_data.at(key);
    }

    MEOJSON_INLINE void object::clear() noexcept { _object_data.clear(); }

    MEOJSON_INLINE bool object::erase(const std::string& key)
    {
        return _object_data.erase(key) > 0 ? true : false;
    }

    MEOJSON_INLINE const std::string object::to_string() const
    {
        std::string str = "{";
        for (const auto& [key, val] : _object_data) {
            str += "\"" + unescape_string(key) + "\":" + val.to_string() + ",";
        }
        if (str.back() == ',') {
            str.pop_back();
        }
        str += "}";
        return str;
    }

    MEOJSON_INLINE const std::string
        object::format(bool ordered,
        std::string shift_str, size_t basic_shift_count) const
    {
        std::string shift;
        for (size_t i = 0; i != basic_shift_count + 1; ++i) {
            shift += shift_str;
        }

        std::string str = "{";
        auto append_kv = [&](const std::string& key, const value& val) {
            str += "\n" + shift + "\"" + unescape_string(key) +
                "\": " + val.format(ordered, shift_str, basic_shift_count + 1) + ",";
        };

        if (ordered) {
            std::vector<raw_object::const_iterator> ordered_data;
            for (auto it = _object_data.cbegin(); it != _object_data.cend(); ++it) {
                ordered_data.emplace_back(it);
            }
            std::sort(ordered_data.begin(), ordered_data.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs->first < rhs->first;
                });
            for (const auto& it : ordered_data) {
                append_kv(it->first, it->second);
            }
        }
        else {
            for (const auto& [key, val] : _object_data) {
                append_kv(key, val);
            }
        }
        if (str.back() == ',') {
            str.pop_back(); // pop last ','
        }

        str += '\n';
        for (size_t i = 0; i != basic_shift_count; ++i) {
            str += shift_str;
        }
        str += '}';
        return str;
    }

    MEOJSON_INLINE bool object::get(const std::string& key,
                                          bool default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_boolean()) {
                return value.as_boolean();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE int object::get(const std::string& key,
                                         int default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_number()) {
                return value.as_integer();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE long object::get(const std::string& key,
                                          long default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_number()) {
                return value.as_long();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE unsigned long object::get(const std::string& key,
                                                   unsigned default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_number()) {
                return value.as_unsigned_long();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE long long object::get(const std::string& key,
                                               long long default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_number()) {
                return value.as_long_long();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE unsigned long long
        object::get(const std::string& key, unsigned long long default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_number()) {
                return value.as_unsigned_long_long();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE float object::get(const std::string& key,
                                           float default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_number()) {
                return value.as_float();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE double object::get(const std::string& key,
                                            double default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_number()) {
                return value.as_double();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE long double object::get(const std::string& key,
                                                 long double default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_number()) {
                return value.as_long_double();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE const std::string object::get(const std::string& key,
                                                 std::string default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_string()) {
                return value.as_string();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE const std::string object::get(const std::string& key,
                                                 const char* default_value) const
    {
        if (contains(key)) {
            value value = _object_data.at(key);
            if (value.is_string()) {
                return value.as_string();
            }
            else {
                return default_value;
            }
        }
        else {
            return default_value;
        }
    }

    MEOJSON_INLINE const value& object::get(const std::string& key) const
    {
        if (contains(key)) {
            return _object_data.at(key);
        }
        else {
            static value null;
            return null;
        }
    }

    template <typename Type>
    MEOJSON_INLINE std::optional<Type> object::find(const std::string& key) const
    {
        static_assert(std::is_constructible<Type, value>::value, "Type can NOT be constructed by value");
        auto iter = _object_data.find(key);
        if (iter == _object_data.end()) {
            return std::nullopt;
        }
        return static_cast<Type>(iter->second);
    }

    MEOJSON_INLINE object::iterator object::begin() noexcept
    {
        return _object_data.begin();
    }

    MEOJSON_INLINE object::iterator object::end() noexcept
    {
        return _object_data.end();
    }

    MEOJSON_INLINE object::const_iterator object::begin() const noexcept
    {
        return _object_data.begin();
    }

    MEOJSON_INLINE object::const_iterator object::end() const noexcept
    {
        return _object_data.end();
    }

    MEOJSON_INLINE object::const_iterator object::cbegin() const noexcept
    {
        return _object_data.cbegin();
    }

    MEOJSON_INLINE object::const_iterator object::cend() const noexcept
    {
        return _object_data.cend();
    }

    MEOJSON_INLINE value& object::operator[](const std::string& key)
    {
        return _object_data[key];
    }

    MEOJSON_INLINE value& object::operator[](std::string&& key)
    {
        return _object_data[std::move(key)];
    }

    MEOJSON_INLINE object object::operator|(const object& rhs)&
    {
        object temp = *this;
        temp._object_data.insert(rhs.begin(), rhs.end());
        return temp;
    }

    MEOJSON_INLINE object object::operator|(object&& rhs)&
    {
        object temp = *this;
        //temp._object_data.merge(std::move(rhs._object_data));
        temp._object_data.insert(
            std::make_move_iterator(rhs.begin()),
            std::make_move_iterator(rhs.end()));
        return temp;
    }

    MEOJSON_INLINE object object::operator|(const object& rhs)&&
    {
        _object_data.insert(rhs.begin(), rhs.end());
        return std::move(*this);
    }

    MEOJSON_INLINE object object::operator|(object&& rhs)&&
    {
        //_object_data.merge(std::move(rhs._object_data));
        _object_data.insert(
            std::make_move_iterator(rhs.begin()),
            std::make_move_iterator(rhs.end()));
        return std::move(*this);
    }

    MEOJSON_INLINE object& object::operator|=(const object& rhs)
    {
        _object_data.insert(rhs.begin(), rhs.end());
        return *this;
    }

    MEOJSON_INLINE object& object::operator|=(object&& rhs)
    {
        _object_data.insert(
            std::make_move_iterator(rhs.begin()),
            std::make_move_iterator(rhs.end()));
        return *this;
    }

    //MEOJSON_INLINE object object::operator&(const object& rhs)&
    //{
    //    object temp;
    //    for (const auto& [key, value] : *this) {
    //        if (rhs.contains(key)) {
    //            temp.emplace(key, value);
    //        }
    //    }
    //    return temp;
    //}

    //MEOJSON_INLINE object object::operator&(object&& rhs)&
    //{
    //    object temp;
    //    for (const auto& [key, value] : *this) {
    //        if (rhs.contains(key)) {
    //            temp.emplace(key, value);
    //        }
    //    }
    //    return temp;
    //}

    //MEOJSON_INLINE object object::operator&(const object& rhs)&&
    //{
    //    object temp;
    //    for (auto&& [key, value] : *this) {
    //        if (rhs.contains(key)) {
    //            temp.emplace(key, std::move(value));
    //        }
    //    }
    //    return temp;
    //}

    //MEOJSON_INLINE object object::operator&(object&& rhs)&&
    //{
    //    object temp;
    //    for (auto&& [key, value] : *this) {
    //        if (rhs.contains(key)) {
    //            temp.emplace(key, std::move(value));
    //        }
    //    }
    //    return temp;
    //}

    //MEOJSON_INLINE object& object::operator&=(const object& rhs)
    //{
    //    *this = std::move(*this) & rhs;
    //    return *this;
    //}

    //MEOJSON_INLINE object& object::operator&=(object&& rhs)
    //{
    //    *this = std::move(*this) & std::move(rhs);
    //    return *this;
    //}

    // const raw_object &object::raw_data() const
    // {
    //     return _object_data;
    // }

    MEOJSON_INLINE std::ostream& operator<<(std::ostream& out, const object& obj)
    {
        // TODO: format output

        out << obj.to_string();
        return out;
    }

    template <typename MapType, typename EnableT>
    object::object(MapType map)
    {
        _object_data.insert(
            std::make_move_iterator(map.begin()),
            std::make_move_iterator(map.end()));
    }

    // *************************
    // *     parser declare    *
    // *************************
    class parser
    {
    public:
        ~parser() noexcept = default;

        static std::optional<value> parse(const std::string& content);

    private:
        parser(const std::string::const_iterator& cbegin,
               const std::string::const_iterator& cend) noexcept
            : _cur(cbegin), _end(cend)
        {
            ;
        }

        std::optional<value> parse();
        value parse_value();

        value parse_null();
        value parse_boolean();
        value parse_number();
        // parse and return a value whose type is value_type::String
        value parse_string();
        value parse_array();
        value parse_object();

        // parse and return a std::string
        std::optional<std::string> parse_stdstring();

        bool skip_whitespace() noexcept;
        bool skip_digit();

    private:
        std::string::const_iterator _cur;
        std::string::const_iterator _end;
    };

    // *************************
    // *      utils impl       *
    // *************************

    MEOJSON_INLINE const value invalid_value()
    {
        return value(value::value_type::Invalid, value::var_t());
    }

    MEOJSON_INLINE std::optional<value> parse(const std::string& content)
    {
        return parser::parse(content);
    }

    MEOJSON_INLINE std::ostream& operator<<(std::ostream& out, const value& val)
    {
        // TODO: format output

        out << val.to_string();
        return out;
    }

    // TODO
    //std::istream &operator>>(std::istream &in, value &val)
    //{
    //    return in;
    //}

    MEOJSON_INLINE std::optional<value> open(const std::ifstream& ifs, bool check_bom = false)
    {
        if (!ifs.is_open()) {
            return std::nullopt;
        }
        std::stringstream iss;
        iss << ifs.rdbuf();
        std::string str = iss.str();

        if (check_bom) {
            using uchar = unsigned char;
            static constexpr uchar Bom_0 = 0xEF;
            static constexpr uchar Bom_1 = 0xBB;
            static constexpr uchar Bom_2 = 0xBF;

            if (str.size() >= 3 &&
                static_cast<uchar>(str.at(0)) == Bom_0 &&
                static_cast<uchar>(str.at(1)) == Bom_1 &&
                static_cast<uchar>(str.at(2)) == Bom_2) {
                str.assign(str.begin() + 3, str.end());
            }
        }

        return parse(str);
    }

    template<typename InputFilename>
    MEOJSON_INLINE std::optional<value> open(const InputFilename& filepath, bool check_bom = false)
    {
        static_assert(std::is_constructible<std::ifstream, InputFilename>::value,
            "InputFilename can't be used to construct a std::ifstream");

        std::ifstream ifs(filepath, std::ios::in);
        auto opt = open(ifs, check_bom);
        ifs.close();
        return opt;
    }

    // *************************
    // *      parser impl      *
    // *************************

    MEOJSON_INLINE std::optional<value> parser::parse(const std::string& content)
    {
        return parser(content.cbegin(), content.cend()).parse();
    }

    MEOJSON_INLINE std::optional<value> parser::parse()
    {
        if (!skip_whitespace()) {
            return std::nullopt;
        }

        value result_value;
        switch (*_cur) {
        case '[':
            result_value = parse_array();
            break;
        case '{':
            result_value = parse_object();
            break;
        default: // A JSON payload should be an object or array
            return std::nullopt;
        }

        if (!result_value.valid()) {
            return std::nullopt;
        }

        // After the parsing is complete, there should be no more content other than
        // spaces behind
        if (skip_whitespace()) {
            return std::nullopt;
        }

        return result_value;
    }

    MEOJSON_INLINE value parser::parse_value()
    {
        switch (*_cur) {
        case 'n':
            return parse_null();
        case 't':
        case 'f':
            return parse_boolean();
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return parse_number();
        case '"':
            return parse_string();
        case '[':
            return parse_array();
        case '{':
            return parse_object();
        default:
            return invalid_value();
        }
    }

    MEOJSON_INLINE value parser::parse_null()
    {
        static constexpr std::string_view null_string = "null";

        for (const char& ch : null_string) {
            if (*_cur == ch) {
                ++_cur;
            }
            else {
                return invalid_value();
            }
        }

        return value();
    }

    MEOJSON_INLINE value parser::parse_boolean()
    {
        static constexpr std::string_view true_string = "true";
        static constexpr std::string_view false_string = "false";

        switch (*_cur) {
        case 't':
            for (const char& ch : true_string) {
                if (*_cur == ch) {
                    ++_cur;
                }
                else {
                    return invalid_value();
                }
            }
            return true;
        case 'f':
            for (const char& ch : false_string) {
                if (*_cur == ch) {
                    ++_cur;
                }
                else {
                    return invalid_value();
                }
            }
            return false;
        default:
            return invalid_value();
        }
    }

    MEOJSON_INLINE value parser::parse_number()
    {
        const auto first = _cur;
        if (*_cur == '-') {
            ++_cur;
        }

        // Numbers cannot have leading zeroes
        if (_cur != _end && *_cur == '0' && _cur + 1 != _end &&
            std::isdigit(*(_cur + 1))) {
            return invalid_value();
        }

        if (!skip_digit()) {
            return invalid_value();
        }

        if (*_cur == '.') {
            ++_cur;
            if (!skip_digit()) {
                return invalid_value();
            }
        }

        if (*_cur == 'e' || *_cur == 'E') {
            if (++_cur == _end) {
                return invalid_value();
            }
            if (*_cur == '+' || *_cur == '-') {
                ++_cur;
            }
            if (!skip_digit()) {
                return invalid_value();
            }
        }

        return value(value::value_type::Number, std::string(first, _cur));
    }

    MEOJSON_INLINE value parser::parse_string()
    {
        auto string_opt = parse_stdstring();
        if (!string_opt) {
            return invalid_value();
        }
        return value(value::value_type::String, std::move(string_opt).value());
    }

    MEOJSON_INLINE value parser::parse_array()
    {
        if (*_cur == '[') {
            ++_cur;
        }
        else {
            return invalid_value();
        }

        if (!skip_whitespace()) {
            return invalid_value();
        }
        else if (*_cur == ']') {
            ++_cur;
            // empty array
            return array();
        }

        array::raw_array result;
        result.reserve(4);
        while (true) {
            if (!skip_whitespace()) {
                return invalid_value();
            }

            value val = parse_value();

            if (!val.valid() || !skip_whitespace()) {
                return invalid_value();
            }

            result.emplace_back(std::move(val));

            if (*_cur == ',') {
                ++_cur;
            }
            else {
                break;
            }
        }

        if (skip_whitespace() && *_cur == ']') {
            ++_cur;
        }
        else {
            return invalid_value();
        }

        return array(std::move(result));
    }

    MEOJSON_INLINE value parser::parse_object()
    {
        if (*_cur == '{') {
            ++_cur;
        }
        else {
            return invalid_value();
        }

        if (!skip_whitespace()) {
            return invalid_value();
        }
        else if (*_cur == '}') {
            ++_cur;
            // empty object
            return object();
        }

        object::raw_object result;
        result.reserve(4);
        while (true) {
            if (!skip_whitespace()) {
                return invalid_value();
            }

            auto key_opt = parse_stdstring();

            if (key_opt && skip_whitespace() && *_cur == ':') {
                ++_cur;
            }
            else {
                return invalid_value();
            }

            if (!skip_whitespace()) {
                return invalid_value();
            }

            value val = parse_value();

            if (!val.valid() || !skip_whitespace()) {
                return invalid_value();
            }

            std::string key_escape = escape_string(std::move(key_opt).value());
            result.emplace(std::move(key_escape), std::move(val));

            if (*_cur == ',') {
                ++_cur;
            }
            else {
                break;
            }
        }

        if (skip_whitespace() && *_cur == '}') {
            ++_cur;
        }
        else {
            return invalid_value();
        }

        return object(std::move(result));
    }

    MEOJSON_INLINE std::optional<std::string> parser::parse_stdstring()
    {
        if (*_cur == '"') {
            ++_cur;
        }
        else {
            return std::nullopt;
        }

        const auto first = _cur;
        auto last = _cur;
        bool is_string_end = false;
        while (!is_string_end && _cur != _end) {
            switch (*_cur) {
            case '\t':
            case '\r':
            case '\n':
                return std::nullopt;
            case '\\': {
                if (++_cur == _end) {
                    return std::nullopt;
                }
                switch (*_cur) {
                case '"':
                case '\\':
                case '/':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                case 'u':
                    ++_cur;
                    break;
                default:
                    // Illegal backslash escape
                    return std::nullopt;
                }
                break;
            }
            case '"': {
                last = _cur;
                ++_cur;
                is_string_end = true;
                break;
            }
            default:
                ++_cur;
                break;
            }
        }
        if (_cur == _end) {
            return std::nullopt;
        }

        return std::string(first, last);
    }

    MEOJSON_INLINE bool parser::skip_whitespace() noexcept
    {
        while (_cur != _end) {
            switch (*_cur) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                ++_cur;
                break;
            default:
                return true;
            }
        }
        return false;
    }

    MEOJSON_INLINE bool parser::skip_digit()
    {
        // At least one digit
        if (_cur != _end && std::isdigit(*_cur)) {
            ++_cur;
        }
        else {
            return false;
        }

        while (_cur != _end && std::isdigit(*_cur)) {
            ++_cur;
        }

        if (_cur != _end) {
            return true;
        }
        else {
            return false;
        }
    }

    // *************************
    // *      aux impl         *
    // *************************

    MEOJSON_INLINE std::string unescape_string(std::string str)
    {
        for (size_t pos = 0; pos < str.size(); ++pos) {
            std::string replace_str;
            switch (str[pos]) {
            case '\"':
                replace_str = R"(\")";
                break;
            case '\\':
                replace_str = R"(\\)";
                break;
            case '\b':
                replace_str = R"(\b)";
                break;
            case '\f':
                replace_str = R"(\f)";
                break;
            case '\n':
                replace_str = R"(\n)";
                break;
            case '\r':
                replace_str = R"(\r)";
                break;
            case '\t':
                replace_str = R"(\t)";
                break;
            default:
                continue;
                break;
            }
            str.replace(pos, 1, replace_str);
            ++pos;
        }
        return str;
    }

    MEOJSON_INLINE std::string escape_string(std::string str)
    {
        for (size_t pos = 0; pos + 1 < str.size(); ++pos) {
            if (str[pos] != '\\') {
                continue;
            }
            std::string replace_str;
            switch (str[pos + 1]) {
            case '"':
                replace_str = "\"";
                break;
            case '\\':
                replace_str = "\\";
                break;
            case 'b':
                replace_str = "\b";
                break;
            case 'f':
                replace_str = "\f";
                break;
            case 'n':
                replace_str = "\n";
                break;
            case 'r':
                replace_str = "\r";
                break;
            case 't':
                replace_str = "\t";
                break;
            default:
                return std::string();
                break;
            }
            str.replace(pos, 2, replace_str);
        }
        return str;
    }
} // namespace json
