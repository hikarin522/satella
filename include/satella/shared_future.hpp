#pragma once
#ifndef SATELLA_INCLUDE_SHARED_FUTURE_HPP
#define SATELLA_INCLUDE_SHARED_FUTURE_HPP

#include "future.hpp"
#include "state.hpp"

namespace satella
{

namespace impl
{

template <class T>
class SharedFuture
{
    std::shared_ptr<State<T>> m_state;

public:
    using value_type = typename State<T>::value_type;

    using callback_type = typename State<T>::shared_callback_type;

    SharedFuture(Future<T> &&future):
        m_state(std::move(future.m_state))
    { }

    SharedFuture() = delete;

    ~SharedFuture() = default;

    SharedFuture(const SharedFuture&) = default;

    SharedFuture& operator =(const SharedFuture&) = default;

    SharedFuture(SharedFuture&&) = default;

    SharedFuture& operator =(SharedFuture&&) = default;

    bool has_value()
    const {
        return m_state->has_value();
    }

    decltype(auto) get()
    const {
        if (!m_state) {
            throw std::future_error(std::future_errc::no_state);
        }
        return m_state->unsafe_get_value();
    }

    template <class _T, enable_if_convertible_t<_T, callback_type> = nullptr>
    std::size_t set_callback(_T &&callback)
    {
        if (!m_state) {
            throw std::future_error(std::future_errc::no_state);
        }
        return m_state->set_shared_callback(std::forward<_T>(callback));
    }

    void unset_callback(const std::size_t &it)
    {
        m_state->unset_shared_callback(it);
    }
};

} // namespace impl

template <class T>
using shared_future = impl::SharedFuture<T>;

} // namespace satella

#endif // SATELLA_INCLUDE_SHARED_FUTURE_HPP
