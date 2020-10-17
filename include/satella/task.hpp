#pragma once
#ifndef SATELLA_INCLUDE_TASK_HPP
#define SATELLA_INCLUDE_TASK_HPP

#include <memory>

#include "functional.hpp"
#include "future.hpp"
#include "promise.hpp"
#include "util.hpp"

namespace satella
{

using job_func_t = unique_function<void()&&>;

using post_func_t = function<void(job_func_t&&)>;

namespace impl
{

template <class T, class F, class V>
void invoke_job(Promise<T> &&promise, F &&func, V &&value)
{
    try {
        std::move(promise).set_value(std::forward<F>(func)(std::forward<V>(value)));
    } catch (...) {
        std::move(promise).set_exception(std::current_exception());
    }
}

template <class F, class V>
void invoke_job(Promise<void> &&promise, F &&func, V &&value)
{
    try {
        std::forward<F>(func)(std::forward<V>(value));
        std::move(promise).set_value();
    } catch (...) {
        std::move(promise).set_exception(std::current_exception());
    }
}

template <class T, class F>
void invoke_job(Promise<T> &&promise, F &&func)
{
    try {
        std::move(promise).set_value(std::forward<F>(func)());
    } catch (...) {
        std::move(promise).set_exception(std::current_exception());
    }
}

template <class F>
void invoke_job(Promise<void> &&promise, F &&func)
{
    try {
        std::forward<F>(func)();
        std::move(promise).set_value();
    } catch (...) {
        std::move(promise).set_exception(std::current_exception());
    }
}

template <class PF, class T, class F, class V>
void post_job(PF &&post_func, Promise<T> &&promise, F &&func, V &&value)
{
    std::forward<PF>(post_func)([
        p = std::move(promise), fn = std::forward<F>(func), val = std::forward<V>(value)
    ]() mutable {
        invoke_job(std::move(p), std::move(fn), std::move(val));
    });
}

template <class PF, class T, class F>
void post_job(PF &&post_func, Promise<T> &&promise, F &&func)
{
    std::forward<PF>(post_func)([
        p = std::move(promise), fn = std::forward<F>(func)
    ]() mutable {
        invoke_job(std::move(p), std::move(fn));
    });
}

template <class>
class Task;

template <class T>
class TaskBase
{
protected:
    using value_type = typename Future<T>::value_type;

    post_func_t m_post;

    Future<T> m_future;

    TaskBase() = delete;

    ~TaskBase() = default;

    TaskBase(const TaskBase&) = delete;

    TaskBase& operator =(const TaskBase&) = delete;

    TaskBase(TaskBase&&) = default;

    TaskBase& operator =(TaskBase&&) = default;

public:
    template <class _PF, enable_if_convertible_t<_PF, post_func_t> = nullptr>
    TaskBase(_PF &&post_fn, Future<T> &&future):
        m_post(std::forward<_PF>(post_fn)),
        m_future(std::move(future))
    { }

    decltype(auto) get()
    && {
        return std::move(this->m_future).get();
    }

    template <class _PF, class _F, enable_if_convertible_t<_PF, post_func_t> = nullptr>
    Task<invoke_result_t<_F, value_type&&>> then_catch(
        bool immediately, _PF &&post_fn, _F &&func
    ) && {
        auto &&promise = Promise<invoke_result_t<_F, value_type&&>>();
        auto &&future  = promise.get_future();
        if (immediately) {
            std::move(this->m_future).set_callback([
                p = std::move(promise), fn = std::forward<_F>(func)
            ](value_type &&value) mutable {
                invoke_job(std::move(p), std::move(fn), std::move(value));
            });
        } else {
            std::move(this->m_future).set_callback([
                post_fn, p = std::move(promise), fn = std::forward<_F>(func)
            ](value_type &&value) mutable {
                post_job(std::move(post_fn), std::move(p), std::move(fn), std::move(value));
            });
        }
        return { std::forward<_PF>(post_fn), std::move(future) };
    }

    template <class _PF, class _F, enable_if_convertible_t<_PF, post_func_t> = nullptr>
    auto then_catch(_PF &&post_fn, _F &&func)
    && {
        return std::move(*this).then_catch(
            false, std::forward<_PF>(post_fn), std::forward<_F>(func)
        );
    }

    template <class _F>
    auto then_catch(bool immediately, _F &&func)
    && {
        return std::move(*this).then_catch(
            immediately, std::move(this->m_post), std::forward<_F>(func)
        );
    }

    template <class _F>
    auto then_catch(_F &&func)
    && {
        return std::move(*this).then_catch(
            false, std::move(this->m_post), std::forward<_F>(func)
        );
    }
};

template <class T>
class Task: public TaskBase<T>
{
    using typename TaskBase<T>::value_type;

public:
    using TaskBase<T>::TaskBase;

    template <class _PF, class _F, enable_if_convertible_t<_PF, post_func_t> = nullptr>
    Task<invoke_result_t<_F, T&&>> then(
        bool immediately, _PF &&post_fn, _F &&func
    ) && {
        auto &&promise = Promise<invoke_result_t<_F, T&&>>();
        auto &&future  = promise.get_future();
        if (immediately) {
            std::move(this->m_future).set_callback([
                p = std::move(promise), fn = std::forward<_F>(func)
            ](value_type &&value) mutable {
                if (index(value) == 0) {
                    std::move(p).set_exception(satella::get<0>(std::move(value)));
                    return;
                }
                invoke_job(std::move(p), std::move(fn), satella::get<1>(std::move(value)));
            });
        } else {
            std::move(this->m_future).set_callback([
                post_fn, p = std::move(promise), fn = std::forward<_F>(func)
            ](value_type &&value) mutable {
                if (index(value) == 0) {
                    std::move(p).set_exception(satella::get<0>(std::move(value)));
                    return;
                }
                post_job(
                    std::move(post_fn), std::move(p), std::move(fn),
                    satella::get<1>(std::move(value))
                );
            });
        }
        return { std::forward<_PF>(post_fn), std::move(future) };
    }

    template <class _PF, class _F, enable_if_convertible_t<_PF, post_func_t> = nullptr>
    auto then(_PF &&post_fn, _F &&func)
    && {
        return std::move(*this).then(
            false, std::forward<_PF>(post_fn), std::forward<_F>(func)
        );
    }

    template <class _F>
    auto then(bool immediately, _F &&func)
    && {
        return std::move(*this).then(
            immediately, std::move(this->m_post), std::forward<_F>(func)
        );
    }

    template <class _F>
    auto then(_F &&func)
    && {
        return std::move(*this).then(
            false, std::move(this->m_post), std::forward<_F>(func)
        );
    }

    template <class _T, enable_if_convertible_t<_T, T> = nullptr>
    operator Task<_T>()
    && {
        return std::move(*this).then(true, [](T &&value) -> _T {
            return std::move(value);
        });
    }
};

template <>
class Task<void>: public TaskBase<void>
{
    using typename TaskBase<void>::value_type;

public:
    using TaskBase<void>::TaskBase;

    template <class _T>
    Task(Task<_T> &&task):
        Task(std::move(task).then(true, [](_T&&) { }))
    { }

    template <class _PF, class _F, enable_if_convertible_t<_PF, post_func_t> = nullptr>
    Task<invoke_result_t<_F>> then(bool immediately, _PF &&post_fn, _F &&func)
    && {
        auto &&promise = Promise<invoke_result_t<_F>>();
        auto &&future  = promise.get_future();
        if (immediately) {
            std::move(this->m_future).set_callback([
                p = std::move(promise), fn = std::forward<_F>(func)
            ](value_type &&value) mutable {
                if (value) {
                    std::move(p).set_exception(std::move(value));
                    return;
                }
                invoke_job(std::move(p), std::move(fn));
            });
        } else {
            std::move(this->m_future).set_callback([
                post_fn, p = std::move(promise), fn = std::forward<_F>(func)
            ](value_type &&value) mutable {
                if (value) {
                    std::move(p).set_exception(std::move(value));
                    return;
                }
                post_job(std::move(post_fn), std::move(p), std::move(fn));
            });
        }
        return { std::forward<_PF>(post_fn), std::move(future) };
    }

    template <class _PF, class _F, enable_if_convertible_t<_PF, post_func_t> = nullptr>
    auto then(_PF &&post_fn, _F &&func)
    && {
        return std::move(*this).then(
            false, std::forward<_PF>(post_fn), std::forward<_F>(func)
        );
    }

    template <class _F>
    auto then(bool immediately, _F &&func)
    && {
        return std::move(*this).then(
            immediately, std::move(this->m_post), std::forward<_F>(func)
        );
    }

    template <class _F>
    auto then(_F &&func)
    && {
        return std::move(*this).then(
            false, std::move(this->m_post), std::forward<_F>(func)
        );
    }
};

} // namespace impl

template <class T>
using task = impl::Task<T>;

template <class PF, class F, impl::enable_if_convertible_t<PF, post_func_t> = nullptr>
task<impl::invoke_result_t<F>> make_task(PF &&post_fn, F &&func)
{
    auto &&promise = satella::promise<impl::invoke_result_t<F>>();
    auto &&future  = promise.get_future();
    impl::post_job(post_fn, std::move(promise), std::forward<F>(func));
    return { std::forward<PF>(post_fn), std::move(future) };
}

} // namespace satella

#endif // SATELLA_INCLUDE_TASK_HPP
