#pragma once
#ifndef SATELLA_INCLUDE_UTIL_HPP
#define SATELLA_INCLUDE_UTIL_HPP

#include <stdexcept>
#include <type_traits>

namespace satella
{

class cancelled_error: public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

namespace impl
{

template <class T>
using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template <bool Condition>
using enable_if_t = typename std::enable_if<Condition, std::nullptr_t>::type;

template <class From, class To>
using enable_if_convertible_t = enable_if_t<std::is_convertible<From, To>::value>;

#ifdef __cpp_lib_is_invocable
template <class T, class ...Ts>
using invoke_result_t = std::invoke_result_t<T, Ts...>;
#else
template <class T, class ...Ts>
using invoke_result_t = typename std::result_of<T(Ts...)>::type;
#endif

template <std::size_t N, class T, class ...Ts>
struct get_pack: get_pack<N - 1, Ts...> { };

template <class T, class ...Ts>
struct get_pack<0, T, Ts...>
{
    using type = T;
};

template <std::size_t N, class ...Ts>
using get_pack_t = typename get_pack<N, Ts...>::type;

template <class T>
decltype(auto) const_off(const T &value)
{
    return const_cast<typename std::remove_const<T>::type&>(value);
}

} // namespace impl

} // namespace satella

#endif // SATELLA_INCLUDE_UTIL_HPP
