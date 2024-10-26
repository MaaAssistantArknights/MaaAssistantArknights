#pragma once

#include "AbstractResource.h"

#include <deque>
#include <filesystem>
#include <memory>

#include "AbstractConfigWithTempl.h"
#include "TemplResource.h"
#include "Utils/SingletonHolder.hpp"

namespace asst
{
class ResourceLoader final : public SingletonHolder<ResourceLoader>, public AbstractResource
{
public:
    virtual ~ResourceLoader() override;

    virtual bool load(const std::filesystem::path& path) override;

    void set_connection_extras(const std::string& name, const json::object& diff);
    bool loaded() const noexcept;

public:
    ResourceLoader();

    void cancel();

private:
    void load_thread_func();

    template <Singleton T>
    requires std::is_base_of_v<AbstractResource, T>
    bool load_resource(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path)) {
            return m_loaded;
        }
        return SingletonHolder<T>::get_instance().load(path);
    }

    template <Singleton T>
    requires std::is_base_of_v<AbstractConfigWithTempl, T>
    bool load_resource_with_templ(const std::filesystem::path& path, const std::filesystem::path& templ_dir)
    {
        if (!load_resource<T>(path)) {
            return false;
        }
        static auto& templ_ins = SingletonHolder<TemplResource>::get_instance();
        const auto& required = SingletonHolder<T>::get_instance().get_templ_required();
        templ_ins.set_load_required(required);

        return load_resource<TemplResource>(templ_dir);
    }

    void add_load_queue(AbstractResource& res, const std::filesystem::path& path);

private:
    bool m_loaded = false;
    std::mutex m_entry_mutex;

    // only for async load
    bool m_load_thread_exit = false;
    std::deque<std::pair<AbstractResource*, std::filesystem::path>> m_load_queue;
    std::mutex m_load_mutex;
    std::condition_variable m_load_cv;
    std::thread m_load_thread;
};
} // namespace asst
