#pragma once
#ifndef SATELLA_INCLUDE_FUNCTIONAL_HPP
#define SATELLA_INCLUDE_FUNCTIONAL_HPP

#ifdef __has_include
  #if __has_include("function2/function2.hpp")
    #include "function2/function2.hpp"
  #elif __has_include("function2.hpp")
    #include "function2.hpp"
  #elif __has_include(<function2/function2.hpp>)
    #include <function2/function2.hpp>
  #elif __has_include(<function2.hpp>)
    #include <function2.hpp>
  #elif __has_include("../function2/function2.hpp")
    #include "../function2/function2.hpp"
  #elif __has_include("../function2.hpp")
    #include "../function2.hpp"
  #elif __has_include("../../../function2/fnction2.hpp")
    #include "../../../function2/fnction2.hpp"
  #elif __has_include("../../../fnction2.hpp")
    #include "../../../fnction2.hpp"
  #elif __has_include("../../../function2/include/function2/fnction2.hpp")
    #include "../../../function2/include/function2/fnction2.hpp"
  #else
    #error <function2/function2.hpp> is not found.
  #endif
#else
  #include <function2/function2.hpp>
#endif

namespace satella
{

template <class T>
using function = fu2::function<T>;

template <class T>
using unique_function = fu2::unique_function<T>;

} // namespace satella

#endif // SATELLA_INCLUDE_FUNCTIONAL_HPP
