#pragma once

namespace asst
{
    template <typename T>
    class SingletonHolder
    {
    public:
        static T& get_instance()
        {
            static T unique_instance;
            return unique_instance;
        }
        virtual ~SingletonHolder() = default;

    public:
        SingletonHolder(const SingletonHolder&) = delete;
        SingletonHolder(SingletonHolder&&) = delete;

        SingletonHolder& operator=(const SingletonHolder&) = delete;
        SingletonHolder& operator=(SingletonHolder&&) = delete;

    protected:
        SingletonHolder() = default;
    };
}
