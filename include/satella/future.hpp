#pragma once
#ifndef SATELLA_INCLUDE_FUTURE_HPP
#define SATELLA_INCLUDE_FUTURE_HPP

#include <future>

#include <memory>

#include "state.hpp"
#include "util.hpp"

namespace satella
{

namespace impl
{

template <class T>
class Future
{
    template <class>
    friend class PromiseBase;

    template <class>
    friend class SharedFuture;

    std::shared_ptr<State<T>> m_state;

    template <class _T>
    Future(_T &&state):
        m_state(std::forward<_T>(state))
    { }

public:
    using value_type = typename State<T>::value_type;

    using callback_type = typename State<T>::callback_type;

    Future() = delete;

    ~Future() = default;

    Future(const Future&) = delete;

    Future& operator =(const Future&) = delete;

    Future(Future&&) = default;

    Future& operator =(Future&&) = default;

    bool valid()
    const {
        return m_state;
    }

    decltype(auto) get()
    && {
        if (!m_state) {
            throw std::future_error(std::future_errc::no_state);
        }
        auto state = std::move(m_state);
        return std::move(*state).get_value();
    }

    template <class _T, enable_if_convertible_t<_T, callback_type> = nullptr>
    void set_callback(_T &&callback)
    && {
        if (!m_state) {
            throw std::future_error(std::future_errc::no_state);
        }
        auto state = std::move(m_state);
        state->set_callback(std::forward<_T>(callback));
    }
};

} // namespace impl

template <class T>
using future = impl::Future<T>;

} // namespace satella

#endif // SATELLA_INCLUDE_FUTURE_HPP
