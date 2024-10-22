#pragma once

#include <optional>
#include <string>
#include <variant>

namespace asst
{
template <typename ResultT>
class ResultOrError
{
    using _variant_t = std::variant<ResultT, std::string>;
    bool m_success;
    _variant_t m_result_or_error;

public:
    template <typename... Args>
    requires std::constructible_from<ResultT, Args...>
    constexpr ResultOrError(Args&&... args) :
        m_success(true),
        m_result_or_error(std::in_place_index<0>, std::forward<Args>(args)...)
    {
    }

    constexpr ResultOrError([[maybe_unused]] std::nullopt_t nullopt, std::string result) :
        m_success(false),
        m_result_or_error(std::in_place_index<1>, std::move(result))
    {
    }

    constexpr operator bool() const noexcept { return m_success; }

    constexpr bool has_value() const noexcept { return m_success; }

    constexpr ResultT& operator*() & { return std::get<0>(m_result_or_error); }

    constexpr const ResultT& operator*() const& { return std::get<0>(m_result_or_error); }

    constexpr ResultT&& operator*() && { return std::move(std::get<0>(m_result_or_error)); }

    constexpr const ResultT&& operator*() const&& { return std::move(std::get<0>(m_result_or_error)); }

    constexpr ResultT& value() & { return std::get<0>(m_result_or_error); }

    constexpr const ResultT& value() const& { return std::get<0>(m_result_or_error); }

    constexpr ResultT&& value() && { return std::move(std::get<0>(m_result_or_error)); }

    constexpr const ResultT&& value() const&& { return std::move(std::get<0>(m_result_or_error)); }

    constexpr std::string& error() & { return std::get<1>(m_result_or_error); }

    constexpr const std::string& error() const& { return std::get<1>(m_result_or_error); }

    constexpr std::string&& error() && { return std::move(std::get<1>(m_result_or_error)); }

    constexpr const std::string&& error() const&& { return std::move(std::get<1>(m_result_or_error)); }
};
} // namespace asst
