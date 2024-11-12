#include "Status.h"

std::optional<int64_t> asst::Status::get_number(const std::string& key) const noexcept
{
    if (auto iter = m_number.find(key); iter != m_number.cend()) {
        return iter->second;
    }
    else {
        return std::nullopt;
    }
}

void asst::Status::set_number(std::string key, int64_t value)
{
    m_number.insert_or_assign(std::move(key), value);
}

void asst::Status::clear_number() noexcept
{
    m_number.clear();
}

std::optional<asst::Rect> asst::Status::get_rect(const std::string& key) const noexcept
{
    if (auto iter = m_rect.find(key); iter != m_rect.cend()) {
        return iter->second;
    }
    else {
        return std::nullopt;
    }
}

void asst::Status::set_rect(std::string key, Rect rect)
{
    m_rect.insert_or_assign(std::move(key), rect);
}

void asst::Status::clear_rect() noexcept
{
    m_rect.clear();
}

std::optional<std::string> asst::Status::get_str(const std::string& key) const noexcept
{
    if (auto iter = m_string.find(key); iter != m_string.cend()) {
        return iter->second;
    }
    else {
        return std::nullopt;
    }
}

void asst::Status::set_str(std::string key, std::string value)
{
    m_string.insert_or_assign(std::move(key), std::move(value));
}

void asst::Status::clear_str() noexcept
{
    m_string.clear();
}

std::optional<std::string> asst::Status::get_properties(const std::string& key) const noexcept
{
    if (auto iter = m_properties.find(key); iter != m_properties.cend()) {
        return iter->second;
    }
    else {
        return std::nullopt;
    }
}

void asst::Status::set_properties(std::string key, std::string value)
{
    m_properties.insert_or_assign(std::move(key), std::move(value));
}

void asst::Status::clear_properties() noexcept
{
    m_properties.clear();
}
