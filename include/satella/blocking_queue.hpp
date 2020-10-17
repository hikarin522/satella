#pragma once
#ifndef SATELLA_INCLUDE_BLOCKING_QUEUE_HPP
#define SATELLA_INCLUDE_BLOCKING_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

#include "cancellation.hpp"
#include "optional.hpp"
#include "util.hpp"

namespace satella
{

template <class T, class Container = std::queue<T>, bool SingleConsumer = false>
class blocking_queue
{
    std::mutex m_mtx;

    std::condition_variable m_cv;

    Container m_queue;

    void notify()
    noexcept {
        SingleConsumer ? m_cv.notify_one() : m_cv.notify_all();
    }

    auto get_lock()
    const {
        return std::unique_lock<std::mutex>(impl::const_off(m_mtx));
    }

    void wait(std::unique_lock<std::mutex> &lock)
    {
        m_cv.wait(lock, [this] { return !m_queue.empty(); });
    }

    template <class Rep, class Period>
    bool wait_for(
        std::unique_lock<std::mutex> &lock,
        const std::chrono::duration<Rep, Period> &rel_time
    ) {
        return m_cv.wait_for(lock, rel_time, [this] { return !m_queue.empty(); });
    }

    template <class Clock, class Duration>
    bool wait_until(
        std::unique_lock<std::mutex> &lock,
        const std::chrono::time_point<Clock, Duration> &abs_time
    ) {
        return m_cv.wait_until(lock, abs_time, [this] { return !m_queue.empty(); });
    }

    bool wait(
        std::unique_lock<std::mutex> &lock,
        const cancellation_token &token
    ) {
        return token.wait<SingleConsumer>(m_cv, lock, [this] { return !m_queue.empty(); });
    }

    template <class Rep, class Period>
    bool wait_for(
        std::unique_lock<std::mutex> &lock,
        const cancellation_token &token,
        const std::chrono::duration<Rep, Period> &rel_time
    ) {
        return token.wait_for<SingleConsumer>(m_cv, lock, rel_time, [this] { return !m_queue.empty(); });
    }

    template <class Clock, class Duration>
    bool wait_until(
        std::unique_lock<std::mutex> &lock,
        const cancellation_token &token,
        const std::chrono::time_point<Clock, Duration> &abs_time
    ) {
        return token.wait_until<SingleConsumer>(m_cv, lock, abs_time, [this] { return !m_queue.empty(); });
    }

public:
    blocking_queue() = default;

    ~blocking_queue() = default;

    blocking_queue(const blocking_queue&) = delete;

    blocking_queue& operator =(const blocking_queue&) = delete;

    blocking_queue(blocking_queue&&) = delete;

    blocking_queue& operator =(blocking_queue&&) = delete;

    decltype(auto) empty()
    const {
        std::lock_guard<std::mutex> lock(impl::const_off(m_mtx));
        return m_queue.empty();
    }

    decltype(auto) size()
    const {
        std::lock_guard<std::mutex> lock(impl::const_off(m_mtx));
        return m_queue.size();
    }

private:
    template <class _T, impl::enable_if_convertible_t<_T, T> = nullptr>
    void _push(bool &require_notify, _T &&value)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        require_notify = m_queue.empty();
        m_queue.push(std::forward<_T>(value));
    }

    template <class _T, impl::enable_if_convertible_t<_T, T> = nullptr>
    void _push(_T &&value)
    {
        bool require_notify;
        _push(require_notify, std::forward<_T>(value));
        if (require_notify) {
            notify();
        }
    }

public:
    void push(T &&value)
    {
        _push(std::move(value));
    }

    void push(const T &value)
    {
        _push(value);
    }

private:
    template <class ...Args>
    decltype(auto) _emplace(bool &require_notify, Args&& ...args)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        require_notify = m_queue.empty();
        return m_queue.emplace(std::forward<Args>(args)...);
    }

    template <class ...Args>
    using emplace_t = decltype(m_queue.emplace(std::declval<Args>()...));

    template <class ...Args, impl::enable_if_t<std::is_void<emplace_t<Args...>>::value> = nullptr>
    void _emplace(Args&& ...args)
    {
        bool require_notify;
        _emplace(require_notify, std::forward<Args>(args)...);
        if (require_notify) {
            notify();
        }
    }

    template <class ...Args, impl::enable_if_t<!std::is_void<emplace_t<Args...>>::value> = nullptr>
    emplace_t<Args...> _emplace(Args&& ...args)
    {
        bool require_notify;
        auto &&ret = _emplace(require_notify, std::forward<Args>(args)...);
        if (require_notify) {
            notify();
        }
        return std::forward<decltype(ret)>(ret);
    }

public:
    template <class ...Args>
    decltype(auto) emplace(Args&& ...args)
    {
        return _emplace(std::forward<Args>(args)...);
    }

private:
    T unsafe_pop()
    {
        auto value = std::move(m_queue.front());
        m_queue.pop();
        return std::move(value);
    }

public:
    T pop()
    {
        auto lock = get_lock();
        wait(lock);
        return unsafe_pop();
    }

    template <class Rep, class Period>
    T pop(const std::chrono::duration<Rep, Period> &rel_time)
    {
        auto lock = get_lock();
        if (!wait_for(lock, rel_time)) {
            throw cancelled_error("");
        }
        return unsafe_pop();
    }

    template <class Clock, class Duration>
    T pop(const std::chrono::time_point<Clock, Duration> &abs_time)
    {
        auto lock = get_lock();
        if (!wait_until(lock, abs_time)) {
            throw cancelled_error("");
        }
        return unsafe_pop();
    }

    T pop(const cancellation_token &token)
    {
        auto lock = get_lock();
        if (!wait(lock, token)) {
            throw cancelled_error("");
        }
        return unsafe_pop();
    }

    template <class Rep, class Period>
    T pop(
        const cancellation_token &token,
        const std::chrono::duration<Rep, Period> &rel_time
    ) {
        auto lock = get_lock();
        if (!wait_for(lock, token, rel_time)) {
            throw cancelled_error("");
        }
        return unsafe_pop();
    }

    template <class Clock, class Duration>
    T pop(
        const cancellation_token &token,
        const std::chrono::time_point<Clock, Duration> &abs_time
    ) {
        auto lock = get_lock();
        if (!wait_until(lock, token, abs_time)) {
            throw cancelled_error("");
        }
        return unsafe_pop();
    }

    optional<T> try_pop()
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_queue.empty()) {
            return nullopt;
        }
        return unsafe_pop();
    }
};

template <class T, class Container = std::queue<T>>
using single_consumer_queue = blocking_queue<T, Container, true>;

} // namespace satella

#endif // SATELLA_INCLUDE_BLOCKING_QUEUE_HPP
