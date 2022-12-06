#pragma once

#include "AbstractResource.h"

#include <filesystem>

#include "AbstractConfigWithTempl.h"
#include "TemplResource.h"
#include "Utils/SingletonHolder.hpp"

namespace asst
{
    class ResourceLoader final : public SingletonHolder<ResourceLoader>, public AbstractResource
    {
    public:
        virtual ~ResourceLoader() override = default;

        virtual bool load(const std::filesystem::path& path) override;
        bool loaded() const noexcept;

    private:
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

    private:
        bool m_loaded = false;
    };
} // namespace asst
