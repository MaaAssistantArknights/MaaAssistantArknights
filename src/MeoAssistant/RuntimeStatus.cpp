#include "RuntimeStatus.h"

std::optional<int64_t> asst::RuntimeStatus::get_number(const std::string& key) const noexcept
{
    if (auto iter = m_number.find(key);
        iter != m_number.cend()) {
        return iter->second;
    }
    else {
        return std::nullopt;
    }
}

void asst::RuntimeStatus::set_number(std::string key, int64_t value)
{
    m_number.insert_or_assign(std::move(key), value);
}

void asst::RuntimeStatus::clear_number() noexcept
{
    m_number.clear();
}

std::optional<asst::Rect> asst::RuntimeStatus::get_rect(const std::string& key) const noexcept
{
    if (auto iter = m_rect.find(key);
        iter != m_rect.cend()) {
        return iter->second;
    }
    else {
        return std::nullopt;
    }
}

void asst::RuntimeStatus::set_rect(std::string key, Rect rect)
{
    m_rect.insert_or_assign(std::move(key), rect);
}

void asst::RuntimeStatus::clear_rect() noexcept
{
    m_rect.clear();
}

std::optional<std::string> asst::RuntimeStatus::get_str(const std::string& key) const noexcept
{
    if (auto iter = m_string.find(key);
        iter != m_string.cend()) {
        return iter->second;
    }
    else {
        return std::nullopt;
    }
}

void asst::RuntimeStatus::set_str(std::string key, std::string value)
{
    m_string.insert_or_assign(std::move(key), std::move(value));
}

void asst::RuntimeStatus::clear_str() noexcept
{
    m_string.clear();
}
