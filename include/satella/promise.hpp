#pragma once
#ifndef SATELLA_INCLUDE_PROMISE_HPP
#define SATELLA_INCLUDE_PROMISE_HPP

#include <future>
#include <memory>

#include "future.hpp"
#include "state.hpp"
#include "util.hpp"

namespace satella
{

namespace impl
{

template <class T>
class PromiseBase
{
protected:
    std::shared_ptr<State<T>> m_state;

    PromiseBase() = default;

    ~PromiseBase()
    {
        if (m_state && !m_state->has_value()) {
            std::move(*this).set_exception(std::make_exception_ptr(cancelled_error("")));
        }
    }

    PromiseBase(const PromiseBase&) = delete;

    PromiseBase& operator =(const PromiseBase&) = delete;

    PromiseBase(PromiseBase&&) = default;

    PromiseBase& operator =(PromiseBase&&) = default;

public:
    Future<T> get_future()
    {
        if (this->m_state) {
            throw std::future_error(std::future_errc::future_already_retrieved);
        }
        this->m_state = std::make_shared<State<T>>();
        return { this->m_state };
    }

    template <class _T, enable_if_convertible_t<_T, std::exception_ptr> = nullptr>
    void set_exception(_T &&ep)
    && {
        if (!this->m_state) {
            throw std::future_error(std::future_errc::no_state);
        }
        auto state = std::move(this->m_state);
        state->set_exception(std::forward<_T>(ep));
    }
};

template <class T>
class Promise: public PromiseBase<T>
{
public:
    using PromiseBase<T>::PromiseBase;

    template <class _T, enable_if_convertible_t<_T, T> = nullptr>
    void set_value(_T &&value)
    && {
        if (!this->m_state) {
            throw std::future_error(std::future_errc::no_state);
        }
        auto state = std::move(this->m_state);
        state->set_value(std::forward<_T>(value));
    }
};

template <>
class Promise<void>: public PromiseBase<void>
{
public:
    using PromiseBase<void>::PromiseBase;

    void set_value()
    && {
        if (!this->m_state) {
            throw std::future_error(std::future_errc::no_state);
        }
        auto state = std::move(this->m_state);
        state->set_value();
    }
};

} // namespace impl

template <class T>
using promise = impl::Promise<T>;

} // namespace satella

#endif // SATELLA_INCLUDE_PROMISE_HPP
