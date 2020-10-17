#pragma once
#ifndef SATELLA_INCLUDE_OPTIONAL_HPP
#define SATELLA_INCLUDE_OPTIONAL_HPP

#ifdef __has_include
  #if __has_include(<optional>)
    #include <optional>
    #ifndef __cpp_lib_optional
      #define __cpp_lib_optional
    #endif
  #elif __has_include(<boost/optional.hpp>)
    #include <boost/optional.hpp>
  #elif __has_include("../boost/optional.hpp")
    #include "../boost/optional.hpp"
  #elif __has_include("../../../boost/optional.hpp")
    #include "../../../boost/optional.hpp"
  #elif __has_include("../../../boost/include/boost/optional.hpp")
    #include "../../../boost/include/boost/optional.hpp"
  #else
    #error <boost/optional.hpp> is not found.
  #endif
#elif defined(__cpp_lib_optional)
  #include <optional>
#else
  #include <boost/optional.hpp>
#endif

namespace satella
{

#ifdef __cpp_lib_optional

template <class T>
using optional = std::optional<T>;

inline constexpr std::nullopt_t nullopt = std::nullopt;

#else

template <class T>
using optional = boost::optional<T>;

const boost::none_t nullopt = boost::none;

#endif

} // namespace satella

#endif // SATELLA_INCLUDE_OPTIONAL_HPP
