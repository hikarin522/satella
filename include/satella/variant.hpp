#pragma once
#ifndef SATELLA_INCLUDE_VARIANT_HPP
#define SATELLA_INCLUDE_VARIANT_HPP

#ifdef __has_include
  #if __has_include(<variant>)
    #include <variant>
    #ifndef __cpp_lib_variant
      #define __cpp_lib_variant
    #endif
  #elif __has_include(<boost/variant.hpp>)
    #include <boost/variant.hpp>
  #elif __has_include("../boost/variant.hpp")
    #include "../boost/variant.hpp"
  #elif __has_include("../../../boost/variant.hpp")
    #include "../../../boost/variant.hpp"
  #elif __has_include("../../../boost/include/boost/variant.hpp")
    #include "../../../boost/include/boost/variant.hpp"
  #else
    #error <boost/variant.hpp> is not found.
  #endif
#elif defined(__cpp_lib_variant)
  #include <variant>
#else
  #include <boost/variant.hpp>
#endif

namespace satella
{

#ifdef __cpp_lib_variant

template <class... Ts>
using variant = std::variant<Ts...>;

using monostate = std::monostate;

template <class T, std::size_t N, class ...Args>
constexpr T make_variant(Args&& ...args)
noexcept {
    return T(std::in_place_index<N>, std::forward<Args>(args)...);
}

template <class T>
constexpr auto index(const T &variant)
noexcept {
    return variant.index();
}

template <std::size_t N, class ...Ts>
constexpr decltype(auto) get(variant<Ts...> &value)
noexcept {
    return std::get<N>(value);
}

template <std::size_t N, class ...Ts>
constexpr decltype(auto) get(const variant<Ts...> &value)
noexcept {
    return std::get<N>(value);
}

template <std::size_t N, class ...Ts>
constexpr decltype(auto) get(variant<Ts...> &&value)
noexcept {
    return std::get<N>(std::move(value));
}

template <std::size_t N, class ...Ts>
constexpr decltype(auto) get(const variant<Ts...> &&value)
noexcept {
    return std::get<N>(std::move(value));
}

#else

template <class... Ts>
using variant = boost::variant<Ts...>;

using monostate = boost::blank;

template <class T, std::size_t N, class... Args>
constexpr T make_variant(Args&& ...args)
noexcept {
    return { std::forward<Args>(args)... };
}

template <class T>
constexpr auto index(const T &variant)
noexcept {
    return variant.which();
}

template <std::size_t N, class ...Ts>
constexpr decltype(auto) get(variant<Ts...> &value)
noexcept {
    return boost::get<impl::get_pack_t<N, Ts...>>(value);
}

template <std::size_t N, class ...Ts>
constexpr decltype(auto) get(const variant<Ts...> &value)
noexcept {
    return boost::get<impl::get_pack_t<N, Ts...>>(value);
}

template <std::size_t N, class ...Ts>
constexpr decltype(auto) get(variant<Ts...> &&value)
noexcept {
    return std::move(boost::get<impl::get_pack_t<N, Ts...>>(value));
}

template <std::size_t N, class ...Ts>
constexpr decltype(auto) get(const variant<Ts...> &&value)
noexcept {
    return std::move(boost::get<impl::get_pack_t<N, Ts...>>(value));
}

#endif

} // namespace satella

#endif // SATELLA_INCLUDE_VARIANT_HPP
