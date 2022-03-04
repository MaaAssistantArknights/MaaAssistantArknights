#include "RuntimeStatus.h"

int64_t asst::RuntimeStatus::get_data(const std::string& key) const noexcept
{
    if (auto iter = m_data.find(key);
        iter != m_data.cend()) {
        return iter->second;
    }
    else {
        return 0;
    }
}

bool asst::RuntimeStatus::contains_data(const std::string& key) const noexcept
{
    return m_data.find(key) != m_data.cend();
}

void asst::RuntimeStatus::set_data(std::string key, int64_t value)
{
    m_data[std::move(key)] = value;
}

void asst::RuntimeStatus::clear_data() noexcept
{
    m_data.clear();
}

asst::Rect asst::RuntimeStatus::get_region(const std::string& key) const noexcept
{
    if (auto iter = m_region_of_appeared.find(key);
        iter != m_region_of_appeared.cend()) {
        return iter->second;
    }
    else {
        return Rect();
    }
}

void asst::RuntimeStatus::set_region(std::string key, Rect rect)
{
    m_region_of_appeared[std::move(key)] = std::move(rect);
}
