#pragma once
#ifndef SATELLA_INCLUDE_CANCELLATION_HPP
#define SATELLA_INCLUDE_CANCELLATION_HPP

#include "functional.hpp"
#include "future.hpp"
#include "promise.hpp"
#include "shared_future.hpp"
#include "util.hpp"

namespace satella
{

namespace impl
{

class CancellationToken
{
    shared_future<void> m_future;

public:
    CancellationToken(const shared_future<void> &future):
        m_future(future)
    { }

    CancellationToken() = delete;

    ~CancellationToken() = default;

    CancellationToken(const CancellationToken&) = default;

    CancellationToken& operator =(const CancellationToken&) = default;

    CancellationToken(CancellationToken&&) = default;

    CancellationToken& operator =(CancellationToken&&) = default;

    bool is_cancelled()
    const {
        return m_future.has_value();
    }

    std::size_t set_callback(satella::function<void()&&> &&func)
    {
        return m_future.set_callback([func = std::move(func)](const std::exception_ptr &ep) mutable {
            try {
                if (ep) {
                    std::rethrow_exception(ep);
                }
            } catch (const cancelled_error&) {
                std::move(func)();
            }
        });
    }

    void unset_callback(const std::size_t &it)
    {
        return m_future.unset_callback(it);
    }

    template <bool Single, class Pred>
    bool wait(
        std::condition_variable &cv,
        std::unique_lock<std::mutex> &mtx,
        const Pred &pred
    ) const {
        bool canceled = false;
        auto it = const_off(*this).set_callback([&, pmtx = mtx.mutex()] {
            {
                 std::lock_guard<std::mutex> lock(*pmtx);
                 canceled = true;
            }
            Single ? cv.notify_one() : cv.notify_all();
        });

        cv.wait(mtx, [&] {
            return canceled || pred();
        });

        const_off(*this).unset_callback(it);

        return !canceled || pred();
    }

    template <class Pred>
    bool wait_one(
        std::condition_variable &cv,
        std::unique_lock<std::mutex> &mtx,
        const Pred &pred
    ) const {
        return wait<true>(mtx, cv, pred);
    }

    template <class Pred>
    bool wait_all(
        std::condition_variable &cv,
        std::unique_lock<std::mutex> &mtx,
        const Pred &pred
    ) const {
        return wait<false>(mtx, cv, pred);
    }

    template <bool Single, class Rep, class Period, class Pred>
    bool wait_for(
        std::condition_variable &cv,
        std::unique_lock<std::mutex> &mtx,
        const std::chrono::duration<Rep, Period> &rel_time,
        const Pred &pred
    ) const {
        bool canceled = false;
        auto it = const_off(*this).set_callback([&, pmtx = mtx.mutex()] {
            {
                 std::lock_guard<std::mutex> lock(*pmtx);
                 canceled = true;
            }
            Single ? cv.notify_one() : cv.notify_all();
        });

        auto ret = cv.wait_for(mtx, rel_time, [&] {
            return canceled || pred();
        });

        const_off(*this).unset_callback(it);

        return ret && (!canceled || pred());
    }

    template <class Rep, class Period, class Pred>
    bool wait_for_one(
        std::condition_variable &cv,
        std::unique_lock<std::mutex> &mtx,
        const std::chrono::duration<Rep, Period> &rel_time,
        const Pred &pred
    ) const {
        return wait_for<true>(cv, mtx, rel_time, pred);
    }

    template <class Rep, class Period, class Pred>
    bool wait_for_all(
        std::condition_variable &cv,
        std::unique_lock<std::mutex> &mtx,
        const std::chrono::duration<Rep, Period> &rel_time,
        const Pred &pred
    ) const {
        return wait_for<false>(cv, mtx, rel_time, pred);
    }

    template <bool Single, class Clock, class Duration, class Pred>
    bool wait_until(
        std::condition_variable &cv,
        std::unique_lock<std::mutex> &mtx,
        const std::chrono::time_point<Clock, Duration> &abs_time,
        const Pred &pred
    ) const {
        bool canceled = false;
        auto it = const_off(*this).set_callback([&, pmtx = mtx.mutex()] {
            {
                 std::lock_guard<std::mutex> lock(*pmtx);
                 canceled = true;
            }
            Single ? cv.notify_one() : cv.notify_all();
        });

        auto ret = cv.wait_until(mtx, abs_time, [&] {
            return canceled || pred();
        });

        const_off(*this).unset_callback(it);

        return ret && (!canceled || pred());
    }

    template <class Clock, class Duration, class Pred>
    bool wait_until_one(
        std::condition_variable &cv,
        std::unique_lock<std::mutex> &mtx,
        const std::chrono::time_point<Clock, Duration> &abs_time,
        const Pred &pred
    ) const {
        return wait_until<true>(cv, mtx, abs_time, pred);
    }

    template <class Clock, class Duration, class Pred>
    bool wait_until_all(
        std::condition_variable &cv,
        std::unique_lock<std::mutex> &mtx,
        const std::chrono::time_point<Clock, Duration> &abs_time,
        const Pred &pred
    ) const {
        return wait_until<false>(cv, mtx, abs_time, pred);
    }
};

class CancellationTokenSource
{
    promise<void> m_promise;

    shared_future<void> m_future;

public:
    CancellationTokenSource():
        m_future(m_promise.get_future())
    { }

    ~CancellationTokenSource() = default;

    CancellationToken get_token()
    const {
        return { m_future };
    }

    void cancel()
    {
        std::move(m_promise).set_exception(
            std::make_exception_ptr(cancelled_error(""))
        );
    }
};

} // namespace impl

using cancellation_token = impl::CancellationToken;

using cancellation_token_source = impl::CancellationTokenSource;

} // namespace satella

#endif // SATELLA_INCLUDE_CANCELLATION_HPP
