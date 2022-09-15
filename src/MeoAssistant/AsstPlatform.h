#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace asst::platform
{
    extern const size_t page_size;

    void* aligned_alloc(size_t len, size_t align);
    void aligned_free(void* ptr);

    template <typename TElem>
    requires std::is_trivial_v<TElem>
    class single_page_buffer
    {
        TElem* _ptr = nullptr;

    public:
        single_page_buffer()
        {
            _ptr = reinterpret_cast<TElem*>(aligned_alloc(page_size, page_size));
            if (!_ptr) throw std::bad_alloc();
        }

        explicit single_page_buffer(std::nullptr_t) {}

        ~single_page_buffer()
        {
            if (_ptr) aligned_free(reinterpret_cast<void*>(_ptr));
        }

        // disable copy construct
        single_page_buffer(const single_page_buffer&) = delete;
        single_page_buffer& operator=(const single_page_buffer&) = delete;

        inline single_page_buffer(single_page_buffer&& other) noexcept { std::swap(_ptr, other._ptr); }
        inline single_page_buffer& operator=(single_page_buffer&& other) noexcept
        {
            if (_ptr) {
                aligned_free(reinterpret_cast<void*>(_ptr));
                _ptr = nullptr;
            }
            std::swap(_ptr, other._ptr);
            return *this;
        }

        inline TElem* get() const { return _ptr; }
        inline size_t size() const { return _ptr ? (page_size / sizeof(TElem)) : 0; }
    };
}
