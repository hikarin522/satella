#pragma once
#ifndef SATELLA_INCLUDE_STATE_HPP
#define SATELLA_INCLUDE_STATE_HPP

#include <algorithm>
#include <condition_variable>
#include <future>
#include <mutex>
#include <vector>

#include "functional.hpp"
#include "optional.hpp"
#include "util.hpp"
#include "variant.hpp"

namespace satella
{

namespace impl
{

template <class T, enable_if_t<!std::is_reference<T>::value> = nullptr>
class StateBase
{
public:
    using value_type = T;

    using callback_type = unique_function<void(value_type&&)&&>;

    using shared_callback_type = unique_function<void(const value_type&)&&>;

private:
    std::mutex m_mtx;

    std::condition_variable m_cv;

    variant<
        monostate,
        T,
        callback_type,
        std::vector<shared_callback_type>
    > m_store;

protected:
    StateBase() = default;

    ~StateBase() = default;

    StateBase(const StateBase&) = delete;

    StateBase& operator =(const StateBase&) = delete;

    StateBase(StateBase&&) = delete;

    StateBase& operator =(StateBase&&) = delete;

    bool unsafe_has_value()
    const {
        return index(m_store) == 1;
    }

public:
    bool has_value()
    const {
        std::lock_guard<std::mutex> lock(const_off(m_mtx));
        return unsafe_has_value();
    }

    auto get_lock()
    const {
        return std::unique_lock<std::mutex>(const_off(m_mtx));
    }

    void wait(std::unique_lock<std::mutex> &lock)
    const {
        const_off(m_cv).wait(lock, [this] { return unsafe_has_value(); });
    }

    template <class Rep, class Period>
    bool wait_for(
        std::unique_lock<std::mutex> &lock,
        const std::chrono::duration<Rep, Period> &rel_time
    ) const {
        return const_off(m_cv).wait_for(lock, rel_time, [this] { return unsafe_has_value(); });
    }

    template <class Clock, class Duration>
    bool wait_until(
        std::unique_lock<std::mutex> &lock,
        const std::chrono::time_point<Clock, Duration> &abs_time
    ) const {
        return const_off(m_cv).wait_until(lock, abs_time, [this] { return unsafe_has_value(); });
    }

    void set_value(T &&value)
    {
        auto lock = get_lock();
        switch (index(m_store)) {
        case 0: {
            m_store = std::move(value);
            lock.unlock();
            m_cv.notify_all();
            return;
        }

        case 1:
            throw std::future_error(std::future_errc::promise_already_satisfied);

        case 2: {
            auto func = satella::get<2>(std::move(m_store));
            m_store = monostate();
            lock.unlock();
            std::move(func)(std::move(value));
            return;
        }

        case 3: {
            auto funcs = satella::get<3>(std::move(m_store));
            m_store = std::move(value);
            lock.unlock();
            for (auto &&func : std::move(funcs)) {
                if (func) {
                    std::move(func)(satella::get<1>(m_store));
                }
            }
            m_cv.notify_all();
            return;
        }

        default:
            throw std::future_error(std::future_errc::broken_promise);
        }
    }

    T& unsafe_get_value(std::unique_lock<std::mutex> &lock)
    {
        wait(lock);
        return satella::get<1>(m_store);
    }

    const T& unsafe_get_value(std::unique_lock<std::mutex> &lock)
    const {
        wait(lock);
        return satella::get<1>(m_store);
    }

    template <class _T, enable_if_convertible_t<_T, callback_type> = nullptr>
    void set_callback(_T &&callback)
    {
        auto lock = get_lock();
        switch (index(m_store)) {
        case 0:
            m_store = std::forward<_T>(callback);
            return;

        case 1: {
            auto &&value = satella::get<1>(std::move(m_store));
            lock.unlock();
            std::forward<_T>(callback)(std::move(value));
            return;
        }

        default:
            throw std::future_error(std::future_errc::broken_promise);
        }
    }

    template <class _T, enable_if_convertible_t<_T, shared_callback_type> = nullptr>
    std::size_t set_shared_callback(_T &&callback)
    {
        auto lock = get_lock();
        switch (index(m_store)) {
        case 0: {
            std::vector<shared_callback_type> vec;
            vec.emplace_back(std::forward<_T>(callback));
            m_store = std::move(vec);
            return 1;
        }

        case 1: {
            const auto &value = satella::get<1>(m_store);
            lock.unlock();
            std::forward<_T>(callback)(value);
            return 0;
        }

        case 3: {
            auto &vec = satella::get<3>(m_store);
            auto it = std::find(vec.begin(), vec.end(), nullptr);
            if (it == vec.end()) {
                vec.emplace_back(std::forward<_T>(callback));
                return vec.size();
            }
            *it = std::forward<_T>(callback);
            return std::distance(vec.begin(), it) + 1;
        }

        default:
            throw std::future_error(std::future_errc::broken_promise);
        }
    }

    void unset_shared_callback(const std::size_t &it)
    {
        if (it == 0) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mtx);
        switch (index(m_store)) {
        case 1:
            return;

        case 3:
            satella::get<3>(m_store).at(it - 1) = nullptr;
            return;

        default:
            throw std::future_error(std::future_errc::broken_promise);
        }
    }
};

template <class T>
class State: public StateBase<variant<std::exception_ptr, T>>
{
#ifndef __cpp_lib_variant
    static_assert(
        !std::is_same<std::exception_ptr, remove_cvref_t<T>>::value,
        "boost::variant does not support the same type"
    );
#endif

    using base = StateBase<variant<std::exception_ptr, T>>;

public:
    using typename base::value_type;

    using StateBase<variant<std::exception_ptr, T>>::StateBase;

    template <class _T, enable_if_convertible_t<_T, T> = nullptr>
    void set_value(_T &&value)
    {
        base::set_value(make_variant<value_type, 1>(std::forward<_T>(value)));
    }

    template <class _T, enable_if_convertible_t<_T, std::exception_ptr> = nullptr>
    void set_exception(_T &&ep)
    {
        base::set_value(make_variant<value_type, 0>(std::forward<_T>(ep)));
    }

    T get_value()
    && {
        auto lock   = base::get_lock();
        auto &value = base::unsafe_get_value(lock);
        if (index(value) == 0) {
            std::rethrow_exception(satella::get<0>(value));
        }
        return satella::get<1>(std::move(value));
    }

    const T& unsafe_get_value()
    const {
        auto lock   = base::get_lock();
        auto &value = base::unsafe_get_value(lock);
        if (index(value) == 0) {
            std::rethrow_exception(satella::get<0>(value));
        }
        return satella::get<1>(value);
    }
};

template <class T>
class State<T&>: public StateBase<variant<std::exception_ptr, std::reference_wrapper<T>>>
{
    using base = StateBase<variant<std::exception_ptr, std::reference_wrapper<T>>>;

public:
    using typename base::value_type;

    using StateBase<variant<std::exception_ptr, std::reference_wrapper<T>>>::StateBase;

    void set_value(T &value)
    {
        base::set_value(make_variant<value_type, 1>(std::ref(value)));
    }

    template <class _T, enable_if_convertible_t<_T, std::exception_ptr> = nullptr>
    void set_exception(_T &&ep)
    {
        base::set_value(make_variant<value_type, 0>(std::forward<_T>(ep)));
    }

    T& get_value()
    && {
        auto lock   = base::get_lock();
        auto &value = base::unsafe_get_value(lock);
        if (index(value) == 0) {
            std::rethrow_exception(satella::get<0>(value));
        }
        return satella::get<1>(value);
    }

    const T& unsafe_get_value()
    const {
        auto lock   = base::get_lock();
        auto &value = base::unsafe_get_value(lock);
        if (index(value) == 0) {
            std::rethrow_exception(satella::get<0>(value));
        }
        return satella::get<1>(value);
    }
};

template <>
class State<void>: public StateBase<std::exception_ptr>
{
    using base = StateBase<std::exception_ptr>;

public:
    using typename base::value_type;

    using StateBase<std::exception_ptr>::StateBase;

    void set_value()
    {
        base::set_value(nullptr);
    }

    void set_exception(std::exception_ptr &&ep)
    {
        base::set_value(std::move(ep));
    }

    void set_exception(const std::exception_ptr &ep)
    {
        base::set_value(std::exception_ptr(ep));
    }

    void get_value()
    && {
        auto lock   = base::get_lock();
        auto &value = base::unsafe_get_value(lock);
        if (value) {
            std::rethrow_exception(value);
        }
    }

    void unsafe_get_value()
    const {
        auto lock   = base::get_lock();
        auto &value = base::unsafe_get_value(lock);
        if (value) {
            std::rethrow_exception(value);
        }
    }
};

} // namespace impl

} // namespace satella

#endif // SATELLA_INCLUDE_STATE_HPP
